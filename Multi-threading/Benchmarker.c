// Yejee Jenny Lee
// April 8th, 2019

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <time.h>

#include <unistd.h>
#include <errno.h>

#include "htll/Hashtable.h"
#include "htll/LinkedList.h"

#include "MovieSet.h"
#include "DocIdMap.h"
#include "FileParser.h"
#include "FileCrawler.h"
#include "MovieIndex.h"
#include "Movie.h"
#include "QueryProcessor.h"
#include "MovieReport.h"


DocIdMap docs;
Index docIndex;
Index movie_index;

void runQueryOffSetIndex();
int GetSetOfMoviesByGenre(Index index, char *type);
/**
 * Open the specified file, read the specified row into the
 * provided pointer to the movie.
 */
int CreateMovieFromFileRow(char *file, long rowId, Movie** movie) {
  FILE *fp;

  char buffer[1000];

  fp = fopen(file, "r");

  int i = 0;
  while (i <= rowId) {
    fgets(buffer, 1000, fp);
    i++;
  }
  // taking \n out of the row
  buffer[strlen(buffer)-1] = ' ';
  // Create movie from row
  *movie = CreateMovieFromRow(buffer);
  fclose(fp);
  return 0;
}

void doPrep(char *dir) {
  printf("Crawling directory tree starting at: %s\n", dir);
  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(dir, docs);

  printf("Crawled %d files.\n", NumElemsInHashtable(docs));

  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles_MT(docs, docIndex);
  // ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

void BenchmarkSetOfMovies(DocIdMap docs) {
  HTIter iter = CreateHashtableIterator(docs);
  // Now go through all the files, and insert them into the index.
  HTKeyValue kv;
  HTIteratorGet(iter, &kv);
  LinkedList movie_list  = ReadFile((char*)kv.value);
  movie_index = BuildMovieIndex(movie_list, Type);

  while (HTIteratorHasMore(iter) != 0) {
    HTIteratorGet(iter, &kv);
    movie_list = ReadFile((char*)kv.value);
    AddToMovieIndex(movie_index, movie_list, Type);
    HTIteratorNext(iter);
  }

  printf("%d entries in the index.\n", NumElemsInHashtable(movie_index->ht));
  DestroyHashtableIterator(iter);
}

int GetSetOfMoviesByGenre(Index index, char *type) {
  HTKeyValue kvp;
  int result = LookupInHashtable(index->ht,
                                 FNVHash64((unsigned char*)type,
                                            strlen(type)), &kvp);
  int count = 0;
  if (result < 0) {
    printf("No such type exists");
    return 1;
  }
  SetOfMovies movie_list = (SetOfMovies)kvp.value;
  LLIter movieIter = CreateLLIter(movie_list->movies);
  if (movieIter == NULL) {
    return 1;
  }
  Movie *movie;
  LLIterGetPayload(movieIter, (void**)&movie);
  if (strstr(movie->title, "seattle")) {
    for (int i = 0; i < NUM_GENRES; i++) {
      if (movie->genres[i] == NULL) {
        break;
      } else if ((strcmp(movie->genres[i], "Romance") == 0) ||
                 (strcmp(movie->genres[i], "Comedy") == 0)) {
        printf(" %s, %s \n", movie->type, movie->title);
        count++;
        break;
      }
    }
  }
  while (LLIterHasNext(movieIter) == 1) {
    LLIterNext(movieIter);
    LLIterGetPayload(movieIter, (void**)&movie);
    if (strstr(movie->title, "seattle")) {
      for (int i = 0; i < NUM_GENRES; i++) {
        if (movie->genres[i] == NULL) {
          break;
        } else if ((strcmp(movie->genres[i], "Romance") == 0) ||
                   (strcmp(movie->genres[i], "Comedy") == 0)) {
          printf(" %s, %s \n", movie->type, movie->title);
          count++;
          break;
        }
      }
    }
  }
  free(movieIter);
  if (count == 0) {
    printf("\nThere is no movie that has term Seattle and ROM/COM.\n");
  }
  printf("count: %d\n", count);
  return 0;
}

void BenchmarkMovieSet(DocIdMap docs) {
  // Create the index
  docIndex = CreateIndex();

  // Index the files
  printf("Parsing and indexing files...\n");
  ParseTheFiles_MT(docs, docIndex);
  // ParseTheFiles(docs, docIndex);
  printf("%d entries in the index.\n", NumElemsInHashtable(docIndex->ht));
}

void WriteFile(FILE *file) {
  int buffer_size = 1000;
  char buffer[buffer_size];

  while (fgets(buffer, buffer_size, file) != NULL) {
    printf("%s\n", buffer);
  }
}


void runQueryOffSetIndex() {
  SearchResultIter results = FindMovies(docIndex, "seattle");
  int count = 0;
  if (results == NULL) {
    printf("No results for this term. Please try another.\n");
    return;
  } else {
    SearchResult sr = (SearchResult)malloc(sizeof(*sr));
    if (sr == NULL) {
      printf("Couldn't malloc SearchResult in main.c\n");
      return;
    }
    int result;
    char *filename;
    SearchResultGet(results, sr);
    filename = GetFileFromId(docs, sr->doc_id);
    Movie *movie;
    CreateMovieFromFileRow(filename, sr->row_id, &movie);

    for (int i = 0; i < NUM_GENRES; i++) {
        if (movie->genres[i] == NULL) {
          break;
        }
        if ((strcmp(movie->genres[i], "Romance") == 0)
           || (strcmp(movie->genres[i], "Comedy") == 0)) {
          count++;
          printf("%s \n", movie->title);
        }
    }
    DestroyMovie(movie);
    while (SearchResultIterHasMore(results) != 0) {
      result =  SearchResultNext(results);
      if (result < 0) {
        printf("error retrieving result\n");
        break;
      }
      SearchResultGet(results, sr);
      char *filename = GetFileFromId(docs, sr->doc_id);
      CreateMovieFromFileRow(filename, sr->row_id, &movie);
      for (int i = 0; i < NUM_GENRES; i++) {
        if (movie->genres[i] == NULL) {
          break;
        }
        if ((strcmp(movie->genres[i], "Romance") == 0)
         || (strcmp(movie->genres[i], "Comedy") == 0)) {
          count++;
          printf("%s \n", movie->title);
        }
      }
      DestroyMovie(movie);
    }
    free(sr);
    DestroySearchResultIter(results);
  }
  if (count == 0) {
    printf("\nThere is no movie that has term Seattle and ROM/COM.\n");
  }
  printf("count :%d \n", count);
}

/*
 * Measures the current (and peak) resident and virtual memories
 * usage of your linux C process, in kB
 */
void getMemory() {
  //    int* currRealMem, int* peakRealMem,
  //    int* currVirtMem, int* peakVirtMem) {

  int currRealMem, peakRealMem, currVirtMem, peakVirtMem;

  // stores each word in status file
  char buffer[1024] = "";

  // linux file contains this-process info
  FILE* file = fopen("/proc/self/status", "r");

  // read the entire file
  while (fscanf(file, " %1023s", buffer) == 1) {
    if (strcmp(buffer, "VmRSS:") == 0) {
      fscanf(file, " %d", &currRealMem);
    }
    if (strcmp(buffer, "VmHWM:") == 0) {
      fscanf(file, " %d", &peakRealMem);
    }
    if (strcmp(buffer, "VmSize:") == 0) {
      fscanf(file, " %d", &currVirtMem);
    }
    if (strcmp(buffer, "VmPeak:") == 0) {
      fscanf(file, " %d", &peakVirtMem);
    }
  }

  fclose(file);

  printf("Cur Real Mem: %d\tPeak Real Mem: %d "
         "\t Cur VirtMem: %d\tPeakVirtMem: %d\n",
         currRealMem, peakRealMem,
         currVirtMem, peakVirtMem);
}

int main(int argc, char *argv[]) {
  // Check arguments
  if (argc != 2) {
    printf("Wrong number of arguments.\n");
    printf("usage: main <directory_to_crawl>\n");
    return 0;
  }
  pid_t pid = getpid();
  printf("Process ID: %d\n", pid);
  getMemory();

  // Create a DocIdMap
  docs = CreateDocIdMap();
  CrawlFilesToMap(argv[1], docs);
  printf("Crawled %d files.\n", NumElemsInHashtable(docs));
  printf("Created DocIdMap\n");

  getMemory();

  clock_t start2, end2;
  double cpu_time_used;


  // =======================
  // Benchmark MovieSet
  printf("\n\nBuilding the OffsetIndex\n");
  start2 = clock();
  BenchmarkMovieSet(docs);
  end2 = clock();
  cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
  printf("Took %f seconds to execute. \n", cpu_time_used);
  printf("Memory usage for OffSetIndex: \n");

  getMemory();
  printf("\n Find movies that has seattle and is romance/comedy. \n");
  start2 = clock();
  runQueryOffSetIndex();
  end2 = clock();
  cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
  printf("Took %f seconds to execute. \n", cpu_time_used);
  printf("Memory usage for query:\n");
  getMemory();
  DestroyOffsetIndex(docIndex);
  printf("Destroyed OffsetIndex\n");
  getMemory();
  // =======================

  // ======================
  // Benchmark SetOfMovies
  printf("\n\nBuilding the TypeIndex\n");
  start2 = clock();
  BenchmarkSetOfMovies(docs);
  end2 = clock();
  cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
  printf("Took %f seconds to execute. \n", cpu_time_used);
  printf("Memory usage for TypeIndex: \n");
  getMemory();
  printf("\n Find movies that has seattle and is romance/comedy. \n");
  start2 = clock();
  GetSetOfMoviesByGenre(movie_index, "movie");
  end2 = clock();
  cpu_time_used = ((double) (end2 - start2)) / CLOCKS_PER_SEC;
  printf("Took %f seconds to execute. \n", cpu_time_used);
  printf("Memory usage for query:\n");
  getMemory();
  DestroyTypeIndex(movie_index);
  printf("Destroyed TypeIndex\n");
  getMemory();
  // ======================
  DestroyDocIdMap(docs);
  printf("\n\nDestroyed DocIdMap\n");
  getMemory();
  // =====================
}
