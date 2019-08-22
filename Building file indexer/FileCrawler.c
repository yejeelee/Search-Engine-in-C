/*
 *  Created by Adrienne Slaughter
 *  Yejee Jenny Lee, April 1st, 2019
 *  CS 5007 Spring 2019
 *  Northeastern University, Seattle
 *
 *  This is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  It is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  See <http://www.gnu.org/licenses/>.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

#include "FileCrawler.h"
#include "DocIdMap.h"
#include "LinkedList.h"

/**
 * Iterates through a given directory, and for all files it
 * finds, adds them to the provided DocIdMap to assign a unique 
 * id.
 * The result of this function is a DocIdMap that contains all the files
 * we will want to index and search.
 * \param dir which directory to crawl
 * \param map the DocIdMap to put the filenames in.
 */
void CrawlFilesToMap(const char *dir, DocIdMap map) {
  struct stat s;

  struct dirent **namelist;
  int n;
  n = scandir(dir, &namelist, 0, alphasort);

  // TODO: use namelist to find all the files and put them in map.
  // NOTE: There may be nested folders.
  // Be sure to lookup how scandir works. Don't forget about memory use.
  int file = 2;
  while (file < n) {
    int pathlen = strlen(dir) + strlen(namelist[file]->d_name) + 2;
    char path[pathlen];
    strcpy(path, dir);
    strcat(path, namelist[file]->d_name);
    stat(path, &s);
    if (S_ISDIR(s.st_mode)) {
      strcat(path, "/");
      CrawlFilesToMap(path, map);
    } else {
      char *fullpath = strdup(path);
      PutFileInMap(fullpath, map);
    }
    file++;
  }

  for(int i = 0; i < n; i++) {
    free(namelist[i]);
  }
  free(namelist);
  
}
