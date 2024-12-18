#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {
  char *name;
  uint64_t type;
  char *offset;
  uint8_t *data;
  uint64_t data_size;
} record_t;

typedef struct recordList_t {
  record_t *record;
  struct recordList_t *next;
} recordList_t;

typedef struct bytetree_t {
  struct bytetree_t *node[256];
  recordList_t *records;
} bytetree_t;

uint64_t count_lines(FILE *file, uint64_t *maxsize) {
  uint64_t count = 0;
  int c;
  uint64_t linesize = 0;
  *maxsize = 0;
  while ((c = fgetc(file)) != EOF) {
    if (c == '\n') {
      count++;
      if (*maxsize < linesize) {
        *maxsize = linesize;
        linesize = 0;
      }
    } else {
      linesize++;
    }
  }
  if (*maxsize < linesize) {
    *maxsize = linesize;
  }
  return count;
}
record_t parse_record(const char *rec) {
  record_t record = {0};
  char *rec_copy = strdup(rec);
  char *token = NULL;
  token = strtok(rec_copy, ":");
  if (token != NULL) {
    record.name = strdup(token);
  }
  token = strtok(NULL, ":");
  if (token != NULL) {
    record.type = strtoull(token, NULL, 10);
  }
  token = strtok(NULL, ":");
  if (token != NULL) {
    record.offset = strdup(token);
  }
  token = strtok(NULL, ":");
  if (token != NULL) {
    record.data_size = strlen(token) / 2;
    record.data = malloc(sizeof(uint8_t) * record.data_size);
    for (uint64_t i = 0; i < record.data_size; i++) {
      sscanf(token + 2 * i, "%2hhx", &record.data[i]);
    }
  }
  return record;
}
bytetree_t *create_bytetree() {
  bytetree_t *tree = malloc(sizeof(bytetree_t));
  tree->records = NULL;
  for (uint64_t i = 0; i < 256; i++) {
    tree->node[i] = NULL;
  }
  return tree;
}
void append_record(bytetree_t *tree, record_t *record) {
  recordList_t *newRecord = malloc(sizeof(recordList_t));
  newRecord->record = record;
  newRecord->next = NULL;
  bytetree_t *current = tree;
  for (uint64_t i = 0; i < record->data_size; i++) {
    if (current->node[record->data[i]] == NULL) {
      current->node[record->data[i]] = create_bytetree();
    }
    current = current->node[record->data[i]];
  }
  if (current->records == NULL) {
    current->records = newRecord;
  } else {
    recordList_t *currentRecord = current->records;
    while (currentRecord->next != NULL) {
      currentRecord = currentRecord->next;
    }
    currentRecord->next = newRecord;
  }
}
recordList_t *search_record(bytetree_t *tree, uint8_t *suspective_data,
                            uint64_t suspective_data_size, uint64_t offset) {
  bytetree_t *current = tree;
  for (uint64_t i = 0;
       (i < suspective_data_size) && (offset + i < suspective_data_size); i++) {
    if (current->records != NULL) {
      return current->records;
    }
    if (current->node[suspective_data[offset + i]] == NULL) {
      return NULL;
    }
    current = current->node[suspective_data[offset + i]];
  }
  return NULL;
}
void free_record(record_t *record) {
  free(record->name);
  free(record->offset);
  free(record->data);
}
void free_bytetree(bytetree_t *tree) {
  for (uint64_t i = 0; i < 256; i++) {
    if (tree->node[i] != NULL) {
      free_bytetree(tree->node[i]);
    }
  }
  recordList_t *current = tree->records;
  while (current != NULL) {
    recordList_t *next = current->next;
    free_record(current->record);
    free(current);
    current = next;
  }
  free(tree);
}
int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s <file>.ndb <suspective file>\n", argv[0]);
    return 1;
  }
  FILE *DBfile = fopen(argv[1], "r");
  if (DBfile == NULL) {
    printf("Error: Could not open file %s\n", argv[1]);
    return 1;
  }
  FILE *suspectiveFile = fopen(argv[2], "r");
  if (suspectiveFile == NULL) {
    printf("Error: Could not open file %s\n", argv[2]);
    return 1;
  }
  uint64_t m;
  uint64_t lines = count_lines(DBfile, &m);
  // for debug
  lines = 30;
  printf("detected %lu records\n", lines);
  record_t *records = malloc(sizeof(record_t) * lines);
  char *line = malloc(sizeof(char) * m);
  rewind(DBfile);
  uint64_t i = 0;
  while (fgets(line, sizeof(char) * m, DBfile)) {

    line[strcspn(line, "\n")] = '\0';
    record_t record = parse_record(line);
    records[i] = record;
    i++;
    if (i >= lines) {
      break;
    }
  }
  for (uint64_t j = 0; j < lines; j++) {
    printf("Record %lu\n", j);
    printf("Name: %s\n", records[j].name);
    printf("Type: %lu\n", records[j].type);
    printf("Offset: %s\n", records[j].offset);
    printf("Data: ");
    for (uint64_t k = 0; k < records[k].data_size; k++) {
      printf("%02x", records[k].data[k]);
    }
    printf("\n");
  }
  fclose(DBfile);
  // 前処理
  bytetree_t *dataset_tree = create_bytetree();
  for (uint64_t j = 0; j < lines; j++) {
    append_record(dataset_tree, &records[j]);
  }
  // 本処理
  fseek(suspectiveFile, 0, SEEK_END);
  int64_t Isuspective_size = ftell(suspectiveFile);
  if (Isuspective_size < 0) {
    printf("Error: Could not get file size\n");
    return 1;
  }
  uint64_t suspective_size = (uint64_t)Isuspective_size;
  rewind(suspectiveFile);
  uint8_t *suspective_data = malloc(sizeof(uint8_t) * suspective_size);
  fread(suspective_data, sizeof(uint8_t), suspective_size, suspectiveFile);
  for (uint64_t j = 0; j < suspective_size; j++) {
    recordList_t *records =
        search_record(dataset_tree, suspective_data, suspective_size, j);
    if (records != NULL) {
      recordList_t *current = records;
      while (current != NULL) {
        printf("Found record %s at offset %lu\n", current->record->name, j);
        current = current->next;
      }
      break;
    }
  }
  // 後処理
  for (uint64_t j = 0; j < lines; j++) {
    free_record(&records[j]);
  }
  free_bytetree(dataset_tree);
  free(records);
  free(line);
}