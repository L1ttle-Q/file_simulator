/*file_simulator.cpp

author: L1ttle-Q
date: 2025-6-6
update: 2025-6-9

*/

#include <cstdio>
#include <cstring>
#include <string>
#include <cmath>
#include <map>

#include <sys/stat.h>
#include <sys/types.h>
#ifdef _WIN32
#include <direct.h>
#endif

#include "file_simulator.h"
#include "file_save.h"

using std::string;
using namespace file_simulator_operation;

// input
const int BUF_MAX = 256;
const string OperationStr[] = {"tree", "treeall", "pwd", "ls", "create", "write", "read", "mkdir",
                               "delete", "deldir", "append", "cp", "rename", "chmod", "cd", "export", "import", "exit"};
char buf[BUF_MAX * 3];
std::map<string, int> OperationDict;

// memory simulator
Strategy strategy = first_fit;
bool Mem_op_print = false;

// file simulator
file_control_block::~file_control_block()
{
    File_simulator::mem->free_by_locate(pfile);
}
Memory_simulator* File_simulator::mem = nullptr;
char* File_simulator::MEMORY = nullptr;

File_simulator* file_simulator;
// file save
const char addr_saved[] = "saved";
FILE* FILE_ISTREAM = NULL;
FILE* FILE_OSTREAM = NULL;

void to_lower(char* s, const int& len)
{
    for (int i = 0; i < len; i++)
        if (s[i] >= 'A' && s[i] <= 'Z')
            s[i] += ('a' - 'A');
}
int cnt_space(const char* s, const int& offset, const int& len)
{
    int t = offset;
    while (t < len && s[t] == ' ') t++;
    return t - offset;
}

void Make_saved_dir()
{
#ifdef _WIN32
    _mkdir(addr_saved);
#else
    mkdir(addr_saved, 0777);
#endif
    printf("created dir %s (ignore if exists)\n", addr_saved);
}

void Init()
{
    file_simulator = new File_simulator();
    constexpr static int len_operationStr = sizeof(OperationStr) / sizeof(OperationStr[0]);
    for (int i = 0; i < len_operationStr; i++)
        OperationDict[OperationStr[i]] = i;
}

int main()
{
    Init();
    printf("\nFile Simulator: (strategy -- first fit)\n");

    bool ext = false;
    File_simulator* new_simulator;
    while (true)
    {
        if (ext) break;

        printf("\n>>> ");
        fgets(buf, BUF_MAX, stdin);
        int len = strlen(buf);
        int off = cnt_space(buf, 0, len);
        if (buf[len - 1] == '\n') buf[len - 1] = 0, len--;

        char op[25];
        char str1[BUF_MAX], str2[BUF_MAX];
        int num = 0;
        sscanf(buf + off, "%s", op);
        off += strlen(op); off += cnt_space(buf, off, len);

        if (OperationDict.find(string(op)) == OperationDict.end())
        {
            printf("Invalid operation.\n");
            continue;
        }
        int parsed = 0;
        switch(OperationDict.at(string(op)))
        {
            case Tree:
                file_simulator->show_tree();
            break;

            case Treeall:
                file_simulator->show_all();
            break;

            case Pwd:
                file_simulator->pwd();
            break;
            
            case Ls:
                file_simulator->ls();
            break;

            case Create:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->create(str1)) printf("success!\n");
            break;

            case Write:
                parsed = sscanf(buf + off, "%s %s", str1, str2);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (parsed < 2)
                {
                    printf("Invalid input: missing content.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->write(str1, str2)) printf("success!\n");
            break;

            case Read:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->read(str1)) printf("success!\n");
            break;

            case Mkdir:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->mkdir(str1)) printf("success!\n");
            break;

            case Delete:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->delete_file(str1)) printf("success!\n");
            break;

            case Deldir:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->delete_folder(str1)) printf("success!\n");
            break;

            case Append:
                parsed = sscanf(buf + off, "%s %s", str1, str2);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (parsed < 2)
                {
                    printf("Invalid input: missing content.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->append(str1, str2)) printf("success!\n");
            break;

            case Cp:
                parsed = sscanf(buf + off, "%s %s", str1, str2);
                if (parsed < 1)
                {
                    printf("Invalid input: missing source file.\n");
                    continue;
                }
                if (parsed < 2)
                {
                    printf("Invalid input: missing dstination file.\n");
                    continue;
                }
                if (file_simulator->cp(str1, str2)) printf("success!\n");
            break;

            case Rename:
                parsed = sscanf(buf + off, "%s %s", str1, str2);
                if (parsed < 1)
                {
                    printf("Invalid input: missing old name.\n");
                    continue;
                }
                if (parsed < 2)
                {
                    printf("Invalid input: missing new name.\n");
                    continue;
                }
                if (Reserved(str1) || Reserved(str2))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->rename(str1, str2)) printf("success!\n");
            break;

            case Chmod:
                parsed = sscanf(buf + off, "%s %d", str1, &num);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (parsed < 2)
                {
                    printf("Invalid input: missing number.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->chmod(str1, num)) printf("success!\n");
            break;

            case Cd:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                if (Reserved(str1))
                {
                    printf("Invalid input: name cannot contain ;[]()\"\\{}\n");
                    continue;
                }
                if (file_simulator->cd(str1)) printf("success!\n");
            break;

            case Export:
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }

                Make_saved_dir();
                FILE_OSTREAM = fopen((std::string(addr_saved) + "/" + str1 + ".simsave").c_str(), "w");
                SaveSimulator(file_simulator);
                fclose(FILE_OSTREAM);
                printf("Already save to %s/%s.simsave\n", addr_saved, str1);
            break;

            case Import: // will cover current simulator, recommend saving it at first
                parsed = sscanf(buf + off, "%s", str1);
                if (parsed < 1)
                {
                    printf("Invalid input: missing name.\n");
                    continue;
                }
                printf("Current simulator would be thrown, recommend saving it before importing.(y for continue)");
                fgets(str2, BUF_MAX, stdin);
                num = strlen(str2);
                if (num > 0 && str2[num - 1] == '\n') str2[num - 1] = '\0';
                if (strlen(str2) > 1 || (str2[0] != 'y' && str2[0] != 'Y')) continue;

                FILE_ISTREAM = fopen((std::string(addr_saved) + "/" + str1 + ".simsave").c_str(), "r");
                if (FILE_ISTREAM == NULL)
                {
                    printf("error: no such file.\n");
                    continue;
                }
                new_simulator = new File_simulator();
                if (!S(new_simulator))
                {
                    printf("Fail to parse.(recovered)\n");
                    delete new_simulator;
                }
                else
                {
                    printf("Parse successfully.\n");
                    File_simulator* tmp = file_simulator;
                    file_simulator = new_simulator;
                    delete tmp;
                }
                fclose(FILE_ISTREAM);
            break;

            case Exit:
                ext = true;
                printf("exit.\n");
            break;

            default:
                ; // should never enter
        }
    }
    return 0;
}