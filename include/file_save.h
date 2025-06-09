/*file_save.h

author: L1ttle-Q
date: 2025-6-8

compress and decompress file simulator

S -> A
G -> AG|BG|e
A -> fdcb{G}
B -> fcb"C"
C -> Ec|e
E -> valid_ch | \any

fdcb -> [name;ctime;mtime;rwx]
fcb -> (name;ctime;mtime;rwx)
valid_ch -> any character except reserved
reserved -> ;[]()"{}
*/

#ifndef _FILE_SAVE_H_
#define _FILE_SAVE_H_

class File_simulator;
class folder_control_block;

#include "file_simulator.h"

class File_simulator_constructor
{
public:
    void SetFolderAttr(File_simulator*, const char*, const time_t, const time_t, const int);
    void SetFileAttr(File_simulator*, const char*, const time_t, const time_t, const int);
    void Setrwx(File_simulator*, const int);

    void SaveSimulator(File_simulator*);
private:
    void SaveSimulator(folder_control_block*);
};

extern FILE* FILE_ISTREAM;
extern FILE* FILE_OSTREAM;

void SaveSimulator(File_simulator*);

bool Reserved(const char&);
bool Reserved(const char*);
bool valid_ch(const char&);
bool S(File_simulator*); // should be a new simulator

#endif /* _FILE_SAVE_H_ */