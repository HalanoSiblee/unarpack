#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#pragma pack(push, 1)
typedef struct {
  uint32_t magic; /* "ARES" */
  uint32_t count; /* total items in index table */
  uint32_t start; /* how much items in root folder */
  uint32_t names; /* names table length */
} res_head;

typedef struct {
  uint32_t offs;
  uint32_t size;
  uint32_t flag;
} res_item;
#pragma pack(pop)

void makedirs(char *path) {
char *s;
  if (path) {
    for (s = path; *s; s++) {
      if ((*s == '\\') || (*s == '/')) {
        *s = 0;
        mkdir(path,0755);
        *s = '/';
      }
    }
  }
}

void dumpdata(char *name, void *data, uint32_t size) {
FILE *f;
  if (name) {
    f = fopen(name, "wb");
    if (f) {
      if (data && size) {
        fwrite(data, size, 1, f);
      }
      fclose(f);
    }
  }
}

void readdata(FILE *fl, void *data, uint32_t size) {
  if (data && size) {
    memset(data, 0, size);
    if (fl) {
      fread(data, size, 1, fl);
    }
  }
}

void deeptree(FILE *fl, res_item *list, char *name, uint32_t from, uint32_t much, char *path, uint32_t pcur) {
uint32_t i, l, num;
char *pre, *ext;
void *p;
  much += from;
  for (i = from; i < much; i++) {
    num = (list[i].flag >> 1) & 0x1FFF;
    pre = &name[(list[i].flag >> 14) & 0x3FFFF];
    if (list[i].flag & 1) {
      /* folder */
      ext = "";
    } else {
      /* file */
      ext = &name[list[i].size >> 23];
    }
    for (l = 0; pre[l]; l++) {
      if (pre[l] == 1) { break; }
    }
    if (pre[l] == 1) {
      pre[l] = 0;
      sprintf(&path[pcur], "%s%u%s%c%s", pre, num, &pre[l + 1], *ext ? '.' : '\\', ext);
      pre[l] = 1;
    } else {
      sprintf(&path[pcur], "%s%c%s", pre, *ext ? '.' : '\\', ext);
    }
    if (list[i].flag & 1) {
      /* dive even deeper */
      deeptree(fl, list, name, list[i].offs, list[i].size, path, strlen(path));
    } else {
      printf("%s\n", path);
      /* save file to disk */
      l = list[i].size & 0x7FFFFF;
      p = malloc(l);
      if (p) {
        fseek(fl, list[i].offs, SEEK_SET);
        readdata(fl, p, l);
        makedirs(path);
        dumpdata(path, p, l);
        free(p);
      } else {
        printf("Warning: not enough memory for output file - skipping.\n");
      }
    }
  }
}

int main(int argc, char *argv[]) {
char *name, s[260];
res_head head;
res_item *list;
uint32_t l;
FILE *fl;
  printf("Midtown Madness .AR extractor v1.0\n(c) CTPAX-X Team 2022\nhttp://www.CTPAX-X.org/\nLinux Port by HalanoSiblee\n\n");
  if (argc != 2) {
    printf("Usage: unarpack <filename.ar>\n\n");
    return(1);
  }
  fl = fopen(argv[1], "rb");
  if (!fl) {
    printf("Error: can't open input file.\n\n");
    return(2);
  }
  readdata(fl, &head, sizeof(head));
  if ((head.magic != 0x53455241) || (!head.count) || (!head.names)) {
    fclose(fl);
    printf("Error: invalid input file format.\n\n");
    return(3);
  }
  /* allocate memory for both tables at once */
  l = (head.count * sizeof(list[0])) + head.names;
  list = (res_item *) malloc(l);
  if (!list) {
    fclose(fl);
    printf("Error: not enough memory for contents table.\n\n");
    return(4);
  }
  readdata(fl, list, l);
  name = (char *) list;
  name += (l - head.names);
  /* dive in file structure */
  deeptree(fl, list, name, 0, head.start, s, 0);
  free(list);
  fclose(fl);
  printf("\ndone\n\n");
  return(0);
}
