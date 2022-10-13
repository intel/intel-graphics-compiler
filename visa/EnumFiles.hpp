/*========================== begin_copyright_notice ============================

Copyright (C) 2017-2021 Intel Corporation

SPDX-License-Identifier: MIT

============================= end_copyright_notice ===========================*/

#ifndef DLL_MODE
#include <algorithm>
#include <string.h>

#if defined _WIN32
#include <tchar.h>
#include <windows.h>
#endif

#ifndef _WIN32
#include "secure_string.h"
#include <dirent.h>
#include <sys/param.h>
#include <unistd.h>

#include <sys/stat.h>
#include <sys/types.h>

#include <stdio.h>
#include <unistd.h>

#ifndef MAXPATHLEN
#include <limits.h>
#define MAXPATHLEN PATH_MAX
#endif
#endif

#ifndef SNPRINTF
#ifdef _WIN32
#define SNPRINTF(dst, size, ...) sprintf_s((dst), (size), __VA_ARGS__)
#else
#define SNPRINTF(dst, size, ...) snprintf((dst), (size), __VA_ARGS__)
#endif
#endif

void ConvertPathToAbsolute(std::string &currentPath);

void dirListFiles(const char *startDir, std::list<std::string> &inputFileList);

/*****************************************************************************\

Function: split

Description: Creates separate strings from string s by given deliminator delim

Input:

Output: splited vector

\*****************************************************************************/
std::vector<std::string> &split(const std::string &s, char delim,
                                std::vector<std::string> &elems) {
  std::stringstream ss(s);
  std::string item;
  while (std::getline(ss, item, delim)) {
    elems.push_back(item);
  }
  return elems;
}

/*****************************************************************************\

Function: split

Description: Creates separate strings from string s by given deliminator delim

Input:

Output: splited vector

\*****************************************************************************/
std::vector<std::string> split(const std::string &s, char delim) {
  std::vector<std::string> elems;
  return split(s, delim, elems);
}

unsigned int PrepareInput(const std::string inputString,
                          std::list<std::string> &preparedList) {
  std::vector<std::string> inputSeparatedList;
  inputSeparatedList = split(inputString, ',');

  for (unsigned int i = 0; i < inputSeparatedList.size(); i++) {
#ifdef _WIN32
    ConvertPathToAbsolute(inputSeparatedList[i]);
    if (inputSeparatedList[i] == "") {
      continue;
    }
#endif
    dirListFiles((const char *)inputSeparatedList[i].c_str(), preparedList);
  }

  preparedList.sort();
  preparedList.unique();
  return static_cast<unsigned>(preparedList.size());
}

// Function tolower() returns int, function std::transform(...) takes tolower()
// as a parameter Visual Studio 2012 generates warning "conversion from 'char'to
// 'int' possible loss of data" and and treat it as error
namespace {
char char_tolower(char x) {
  return static_cast<char>(tolower(static_cast<int>(x)));
}
}; // namespace

#if defined _WIN32
/*****************************************************************************\

Function:

Description: Creates list of shader files from given directory which can
consist:
- whole directories
- single files
- filenames with wildcards
This function expands all of above and recursively search of every
which fit given pattern.

Input:

Output:

\*****************************************************************************/
void dirListFiles(const char *startDir, std::list<std::string> &inputFileList) {
  HANDLE hFind;
  WIN32_FIND_DATA wfd;
  bool isDirectory = false;
  char path[MAX_PATH + 1];
  char temp[MAX_PATH + 1];
  memset(path, 0, MAX_PATH + 1);
  memset(temp, 0, MAX_PATH + 1);
  std::string startDirStr((std::string)startDir);
  if (startDirStr[startDirStr.length() - 1] == '\\') {
    isDirectory = true;
    startDirStr.append("*");
    memcpy_s(path, MAX_PATH, startDirStr.c_str(), startDirStr.length());
    // sprintf_s(path, startDirStr.length(), startDir, "*");
  } else {
    memcpy_s(path, MAX_PATH, startDirStr.c_str(), startDirStr.length());
    // sprintf_s(path, startDirStr.length(), startDir, "*");
  }
  hFind = FindFirstFile(path, &wfd);
  if (hFind == INVALID_HANDLE_VALUE) {
    std::cerr << "vISA: WARNING - no files meet specified criteria: \""
              << startDir << "\"" << std::endl;
    return;
  }
  if (!isDirectory) {
    size_t lastSlash;
    lastSlash = startDirStr.rfind("\\");
    startDirStr.erase(lastSlash + 1);
  }

  bool hasFiles = true;
  while (hasFiles) {
    if ((strncmp(".", wfd.cFileName, 1) != 0) &&
        (strncmp("..", wfd.cFileName, 2) != 0)) {
      if (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
        if (!isDirectory) {
          SNPRINTF(path, "%s%s\\", startDirStr.c_str(), wfd.cFileName);
          dirListFiles(path, inputFileList);
        } else {
          SNPRINTF(path, "%s%s\\", startDir, wfd.cFileName);
          dirListFiles(path, inputFileList);
        }
      } else {
        if (!isDirectory) {
          SNPRINTF(temp, "%s%s", startDirStr.c_str(), wfd.cFileName);
          inputFileList.push_back(temp);
          std::transform(inputFileList.back().begin(),
                         inputFileList.back().end(),
                         inputFileList.back().begin(), char_tolower);
        } else {
          SNPRINTF(temp, "%s%s", startDir, wfd.cFileName);
          inputFileList.push_back(temp);
          std::transform(inputFileList.back().begin(),
                         inputFileList.back().end(),
                         inputFileList.back().begin(), char_tolower);
        }
      }
    }
    hasFiles = FindNextFile(hFind, &wfd) == TRUE;
  }
  if (GetLastError() != ERROR_NO_MORE_FILES) {
    std::cerr << "vISA: ERROR - can't find next file for some reason, path: "
              << path << std::endl;
    exit(-1);
  }
  if (FindClose(hFind) == FALSE) {
    std::cerr << "vISA: ERROR - can't close file, path: " << path << std::endl;
    exit(-1);
  }
}

/*****************************************************************************\

Function: ConvertPathToAbsolute

Description: Windows-specific conversion of a given path. If given path is
relative, it's being converted to absolute form, resolving all "../" and "./"
constructions in order to fit OS maximum path length limitation

Input: std::string &currentPath

Output: none

\*****************************************************************************/
void ConvertPathToAbsolute(std::string &currentPath) {
  char pathBuffer[MAX_PATH] = "";
  if (_fullpath(pathBuffer, currentPath.c_str(), MAX_PATH) == NULL) {
    char *errMsg;
    DWORD errCode = GetLastError();
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, errCode, 0, (LPTSTR)&errMsg, 0, NULL);
    std::cerr << "vISA: ERROR - " << (char *)errMsg << std::endl;
    if (errCode == ERROR_FILENAME_EXCED_RANGE) {
      std::cerr << "vISA: Maximum path length in your environment is "
                << MAX_PATH << " chars" << std::endl;
      std::cerr << "filename: " << currentPath.c_str() << " skipped"
                << std::endl;
    }
    //        exit(-1);
  }
  currentPath = (char *)&pathBuffer;
}
#else // defined _WIN32

/*****************************************************************************\

Function:

Description:
Creates list of shader files from given directory.
Exact directory path should be given.
Additional functionality which is implemented for Win OS is not implemented yet.

Input:

Output:

\*****************************************************************************/
int file_select(const struct dirent *entry) {
  if ((strcmp(entry->d_name, ".") == 0) || (strcmp(entry->d_name, "..") == 0))
    return 0;
  else
    return 1;
}
void dirListFiles(const char *startDir, std::list<std::string> &inputFileList) {
  const char *path = startDir;
  char temp[MAXPATHLEN];
  memset(temp, 0, MAXPATHLEN);
  struct stat path_stat;
  if (stat(path, &path_stat) != 0) {
    perror("stat");
    return;
  }
  if (S_ISREG(path_stat.st_mode)) {
    inputFileList.push_back(path);
    return;
  }

  int count, i;
  struct dirent **files;

  printf("vISA: Current Processing Directory = %s \n", path);
  count = scandir(path, &files, file_select, NULL);

  /* If no files found, make a non-selectable menu item */
  if (count <= 0) {
    printf("vISA: WARNING - No files in this directory \n");
    // exit(0);
  }
  printf("vISA: Number of files = %d \n", count);
  for (i = 0; i < count; ++i) {
    sprintf_s(temp, MAXPATHLEN, "%s%s", startDir, files[i]->d_name);
    printf("%s \n", temp);
    inputFileList.push_back(temp);
  }
  printf("\n"); /* flush buffer */
}

#endif // defined _WIN32
#endif // defined DLL_MODE
