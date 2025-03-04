/*
 * Copyright (C) 2021 Liquidaty and the zsv/lib contributors
 * All rights reserved
 *
 * This file is part of zsv/lib, distributed under the license defined at
 * https://opensource.org/licenses/MIT
 */

#include <zsv.h>
#include <zsv/utils/utf8.h>
#include <zsv/utils/signal.h>
#include <zsv/utils/arg.h>
#include <zsv/utils/compiler.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum zsv_2tsv_status {
  zsv_2tsv_status_ok = 0,
  zsv_2tsv_status_out_of_memory
};

#define ZSV_2TSV_BUFF_SIZE 65536

struct static_buff {
  char *buff; // will be ZSV_2TSV_BUFF_SIZE
  size_t used;
  FILE *stream;
};

struct zsv_2tsv_data {
  zsv_parser parser;
  struct static_buff out;
  unsigned char overflowed:1;
  unsigned char _:7;
};

__attribute__((always_inline)) static inline void zsv_2tsv_flush(struct static_buff *b) {
  fwrite(b->buff, b->used, 1, b->stream);
  b->used = 0;
}

static inline void zsv_2tsv_write(struct static_buff *b, const unsigned char *s, size_t n) {
  if(n) {
    if(VERY_UNLIKELY(n + b->used > ZSV_2TSV_BUFF_SIZE)) {
      zsv_2tsv_flush(b);
      if(VERY_UNLIKELY(n > ZSV_2TSV_BUFF_SIZE)) { // n too big, so write directly
        fwrite(s, n, 1, b->stream);
        return;
      }
    }
    // n + used < buff size
    memcpy(b->buff + b->used, s, n);
    b->used += n;
  }
}

// zsv_to_tsv(): convert, if necessary, to text suitable for tab-delimited output.
// - return NULL if nothing to convert
// - else, return allocated char * of converted tsv (caller must free), and update *lenp
// - on error, set *err
__attribute__((always_inline)) static inline
unsigned char *zsv_to_tsv(const unsigned char *utf8, size_t *len, enum zsv_2tsv_status *err) {
  // replace tab, newline and lf with \t, \n or \r or backslash
  size_t do_convert = 0;
  for(size_t i = 0; i < *len; i++) {
    if(VERY_UNLIKELY(utf8[i] == '\t') || VERY_UNLIKELY(utf8[i] == '\n') || VERY_UNLIKELY(utf8[i] == '\r') || VERY_UNLIKELY(utf8[i] == '\\'))
      do_convert++;
  }

  if(LIKELY(do_convert == 0))
    return NULL;

  unsigned char *converted = malloc(*len + do_convert + 1);
  if(!converted)
    *err = zsv_2tsv_status_out_of_memory;
  else {
    size_t j = 0;
    for(size_t i = 0; i < *len; i++) {
      if(UNLIKELY(utf8[i] == '\t' || utf8[i] == '\n' || utf8[i] == '\r' || utf8[i] == '\\')) {
        converted[j++] = '\\';
        converted[j++] = utf8[i] == '\t' ? 't' : utf8[i] == '\n' ? 'n' : utf8[i] == '\r' ? 'r' : '\\';
      } else
        converted[j++] = utf8[i];
    }
    converted[j] = '\0';
    *len = j;
  }
  return converted;
}

__attribute__((always_inline)) static inline
void zsv_2tsv_cell(struct zsv_2tsv_data *data, unsigned char *utf8_value, size_t len, char no_escape) {
  // output cell contents (converted if necessary)
  if(len) {
    enum zsv_2tsv_status err = zsv_2tsv_status_ok;
    if(LIKELY(no_escape)) {
      zsv_2tsv_write(&data->out, utf8_value, len);
      return;
    }
    unsigned char *converted = zsv_to_tsv(utf8_value, &len, &err);
    if(UNLIKELY(converted != NULL)) {
      zsv_2tsv_write(&data->out, converted, len);
      free(converted);
    } else if(VERY_UNLIKELY(err != zsv_2tsv_status_ok))
      ; // handle out-of-memory error!
    else
      zsv_2tsv_write(&data->out, utf8_value, len);
  }
}

void zsv_2tsv_overflow(void *ctx, unsigned char *utf8_value, size_t len) {
  struct zsv_2tsv_data *data = ctx;
  if(len) {
    if(!data->overflowed) {
      fwrite("overflow! ", 1, strlen("overflow! "), stderr);
      fwrite(utf8_value, 1 ,len, stderr);
      fprintf(stderr, "(subsequent overflows will be suppressed)");
      data->overflowed = 1;
    }
  }
}

static void zsv_2tsv_row(void *ctx) {
  struct zsv_2tsv_data *data = ctx;
  unsigned int cols = zsv_column_count(data->parser);
  if(cols) {
    struct zsv_cell cell = zsv_get_cell(data->parser, 0);
    char no_escape = 0;
    if(cols > 1) {
      struct zsv_cell end = zsv_get_cell(data->parser, cols-1);
      unsigned char *start = cell.str;
      size_t row_len = end.str + end.len - start;
      if(!(memchr(start, '\t', row_len) || memchr(start, '\n', row_len) || memchr(start, '\r', row_len) || memchr(start, '\\', row_len)))
        no_escape = 1;
    }
    zsv_2tsv_cell(ctx, cell.str, cell.len, no_escape);

    for(unsigned int i = 1; i < cols; i++) {
      zsv_2tsv_write(&data->out, (const unsigned char *)"\t", 1);
      cell = zsv_get_cell(data->parser, i);
      zsv_2tsv_cell(ctx, cell.str, cell.len, no_escape);
    }
  }
  zsv_2tsv_write(&data->out, (const unsigned char *) "\n", 1);
}

#ifndef APPNAME
#define APPNAME "zsv_2tsv"
#endif


#ifndef MAIN
#define MAIN main
#endif

int MAIN(int argc, const char *argv[]) {
  FILE *f_in = NULL;
  struct zsv_2tsv_data data = { 0 };

  INIT_CMD_DEFAULT_ARGS();

  int err = 0;
  for(int i = 1; !err && i < argc; i++) {
    if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h")) {
      fprintf(stdout, "Usage: " APPNAME " [filename] [-o <output_filename>]\n");
      fprintf(stdout, "  Reads CSV input and converts to tsv");
      err = 1;
    } else if(!strcmp(argv[i], "-o") || !strcmp(argv[i], "--output")) {
      if(++i >= argc)
        fprintf(stderr, "%s option requires a filename value\n", argv[i-1]), err = 1;
      else if(data.out.stream && data.out.stream != stdout)
        fprintf(stderr, "Output file specified more than once\n"), err = 1;
      else if(!(data.out.stream = fopen(argv[i], "wb")))
        fprintf(stderr, "Unable to open for writing: %s\n", argv[i]), err = 1;
    } else {
      if(f_in)
        fprintf(stderr, "Input file specified more than once\n"), err = 1;
      else if(!(f_in = fopen(argv[i], "rb")))
        fprintf(stderr, "Unable to open for reading: %s\n", argv[i]), err = 1;
    }
  }

  if(err) {
    if(f_in)
      fclose(f_in);
    if(data.out.stream)
      fclose(data.out.stream);
    goto exit_2tsv;
  }

  if(!f_in) {
#ifdef NO_STDIN
    fprintf(stderr, "Please specify an input file\n");
    err = 1;
    goto exit_2tsv;
#else
    f_in = stdin;
#endif
  }

  if(!data.out.stream)
    data.out.stream = stdout;

  struct zsv_opts opts = zsv_get_default_opts();
  opts.row = zsv_2tsv_row;
  opts.ctx = &data;
  opts.overflow = zsv_2tsv_overflow;

  opts.stream = f_in;
  if((data.parser = zsv_new(&opts))) {
    char output[ZSV_2TSV_BUFF_SIZE];
    data.out.buff = output;

    zsv_handle_ctrl_c_signal();
    enum zsv_status status;
    while(!zsv_signal_interrupted
          && (status = zsv_parse_more(data.parser)) == zsv_status_ok)
      ;
    zsv_finish(data.parser);
    zsv_delete(data.parser);
    zsv_2tsv_flush(&data.out);
  }

  if(f_in && f_in != stdin)
    fclose(f_in);
  if(data.out.stream)
    fclose(data.out.stream);

 exit_2tsv:
  return err;
}
