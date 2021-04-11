#pragma once 

#ifndef _CW2_UTILS
#define _CW2_UTILS

void sendFileToSocketFd(int fd, char* filename);
void sendEncryFileToSocketFd(int fd, char* filename);
void downloadEncryFileFromSocketFd(int fd, char* filename);
void downloadFileFromSocketFd(int fd, char* filename);
void setBlockingFd(_Bool toggle, int fd);
void xorEnc(char* data);

#endif //_CW2_UTILS