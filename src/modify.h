// -*- coding: koi8-r -*-

// ����������� �� ������� � ������� �������� ��������� bitbucket

#ifndef _MODIFY_H_
#define _MODIFY_H_

#include "structs.h"

void string_add(DESCRIPTOR_DATA * d, char *str);
void string_write(DESCRIPTOR_DATA * d, char **txt, size_t len, int mailto, void *data);
void page_string(DESCRIPTOR_DATA * d, char *str, int keep_internal);
void page_string(DESCRIPTOR_DATA * d, std::string buf);
void print_con_prompt(DESCRIPTOR_DATA * d);

#endif
