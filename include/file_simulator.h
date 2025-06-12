/*file_simulator.h

author: L1ttle-Q
date: 2025-6-5
upd: 2025-6-8

// tree operation ignore access check

operation:
tree // current dir
treeall  // root dir
pwd
ls
create
write <filename> <data>
read <filename>
mkdir <foldername>
delete <filename>
deldir <foldername>
append <filename> <data>
cp <filename> <filename>
rename <filename> <filename>
chmod <filename> <rwx>
cd <foldername>
exit

*/

#ifndef _FILE_SIMULATOR_H_
#define _FILE_SIMULATOR_H_

#include <cstring>
#include <ctime>
#include <iostream>

#include "mem_simulator.h"
#include "file_save.h"

#define min(a, b) ((a) > (b) ? (b) : (a))
#define R 0400
#define W 0200
#define X 0100

const int MAX_NAME_LENGTH = 64;

namespace file_simulator_operation
{
    enum Operation
    {
        Tree, Treeall,
        Pwd, Ls,
        Create, Write,
        Read, Mkdir,
        Delete, Deldir,
        Append, Cp, Rename,
        Chmod, Cd,
        Export, Import,
        Exit
    };
}

class basic_block;
class file_control_block;
class folder_control_block;
class File_simulator;

#ifdef _FILE_SAVE_H_
class File_simulator_constructor;
#endif /* _FILE_SAVE_H_ */

class basic_block
{
protected:
    char name[MAX_NAME_LENGTH];
    time_t ctime, mtime;
    int rwx;

public:
    basic_block* sibling;

    virtual const int Size() const {return 0;};
    virtual ~basic_block() {}

    basic_block(const char* _name, const time_t _ctime, const int _rwx, basic_block* _sibling)
    {
        strncpy(name, _name, MAX_NAME_LENGTH - 1);
        name[MAX_NAME_LENGTH - 1] = '\0';
        ctime = mtime = _ctime;
        rwx = _rwx;
        sibling = _sibling;
    }
    const char* get_name() const {return name;}
    void modify_name(const char* _name)
    {
        strncpy(name, _name, MAX_NAME_LENGTH - 1);
        name[MAX_NAME_LENGTH - 1] = '\0';
    }
    time_t get_ctime() const {return ctime;}
    time_t get_mtime() const {return mtime;}

    void modify_ctime(const time_t _ctime) {ctime = _ctime;}
    void modify_mtime(const time_t _mtime) {mtime = _mtime;}
    const int get_rwx() const {return rwx;}
    void modify_rwx(const int _rwx) {rwx = _rwx;}

};

class folder_control_block : public basic_block
{
public:
    folder_control_block* parent;
    basic_block* ch;

    folder_control_block(const char* _name, const time_t _ctime, const int _rwx,
                         folder_control_block* _parent, basic_block* _sibling):
                         basic_block(_name, _ctime, _rwx, _sibling)
    {
        parent = _parent;
        ch = nullptr;
    }

    ~folder_control_block()
    {
        basic_block* p = ch;
        basic_block* tmp;
        while (p)
        {
            tmp = p;
            p = p->sibling;
            delete tmp;
        }
    }

    const int Size() const
    {
        int res = 0;
        basic_block* p = ch;
        while (p)
        {
            res += p->Size();
            p = p->sibling;
        }
        return res;
    }
};

class file_control_block : public basic_block
{
private:
    int pfile;
    int size;

    friend File_simulator;
#ifdef _FILE_SAVE_H_
    friend File_simulator_constructor;
#endif
public:
    folder_control_block* parent;

    file_control_block(const char* _name, const time_t _ctime, const int _rwx, const int _size,
                       const int _pfile, folder_control_block* _parent, basic_block* _sibling):
                       basic_block(_name, _ctime, _rwx, _sibling)
    {
        parent = _parent;
        size = _size;
        pfile = _pfile;
    }

    ~file_control_block();

    const char* Type() // check file type by postfix
    {
        int len = strlen(name);
        for (int i = len - 2; i >= 0; i--)
            if (name[i] == '.')
                return (name + i + 1);
        return "FILE";
    }
    const int Size() const
    {
        return this->size;
    }
};

class File_simulator
{
private:
    folder_control_block root_folder;
    static Memory_simulator* mem;
    static char* MEMORY;

    folder_control_block* now;

    friend folder_control_block;
    friend file_control_block;
#ifdef _FILE_SAVE_H_
    friend File_simulator_constructor;
#endif

    char* recursive_parent_name(folder_control_block* p)
    {
        static char path[1024];
        if (p == &root_folder) {path[0] = '\0'; return path;}
        char* p_path = recursive_parent_name(p->parent);
        snprintf(path, sizeof(path), "%s/%s", p_path, p->get_name());
        return path;
    }

    void show_tree(folder_control_block* p, int depth)
    {
        printf("%s/\n", p->get_name());
        basic_block* ch = p->ch;
        while (ch)
        {
            for (int i = 1; i <= depth; i++) printf("|  ");
            printf("|--");
            
            if (dynamic_cast<folder_control_block*>(ch))
                show_tree(dynamic_cast<folder_control_block*>(ch), depth + 1);
            else
                printf("%s\n", ch->get_name());
            ch = ch->sibling;
        }
    }

    bool check_name(const char* name)
    {
        basic_block* ch = now->ch;
        while (ch)
        {
            if (!strcmp(ch->get_name(), name)) return true;
            ch = ch->sibling;
        }
        return false;
    }

    file_control_block* find_file(const char* name, folder_control_block* p)
    {
        basic_block* ch = p->ch;
        file_control_block* dst_file = nullptr;
        while (ch)
        {
            if (!strcmp(ch->get_name(), name))
            {
                if ((dst_file = dynamic_cast<file_control_block*>(ch)) == nullptr)
                {
                    fprintf(stderr, "error: %s is a folder.\n", name);
                    return nullptr;
                }
                return dst_file;
            }
            ch = ch->sibling;
        }
        fprintf(stderr, "error: no such file.\n");
        return nullptr;
    }

    folder_control_block* find_folder(const char* name, folder_control_block* p)
    {
        basic_block* ch = p->ch;
        folder_control_block* dst_folder = nullptr;
        while (ch)
        {
            if (!strcmp(ch->get_name(), name))
            {
                if ((dst_folder = dynamic_cast<folder_control_block*>(ch)) == nullptr)
                {
                    fprintf(stderr, "error: %s is a file.\n", name);
                    return nullptr;
                }
                return dst_folder;
            }
            ch = ch->sibling;
        }
        fprintf(stderr, "error: no such folder.\n");
        return nullptr;
    }

    basic_block* find_prev(const basic_block* p, folder_control_block* parent)
    {
        basic_block* tmp = parent->ch;
        while (tmp->sibling)
        {
            if (tmp->sibling == p) return tmp;
            tmp = tmp->sibling;
        }
        return nullptr;
    }

public:
    File_simulator() : root_folder("", std::time(nullptr), 0777, nullptr, nullptr)
    {
        if (!mem)
        {
            mem = new Memory_simulator();
            MEMORY = new char[MEMORY_STORAGY];
        }
        now = &root_folder;
    }
    ~File_simulator() = default;

    void show_tree()
    {
        show_tree(now, 0);
    }

    void pwd()
    {
        printf("%s/\n", recursive_parent_name(now));
    }

    void ls()
    {
        if (!(now->get_rwx() & R))
        {
            fprintf(stderr, "Permission denied.\n");
            return ;
        }

        printf("Total size: %d\n", now->Size());
        basic_block* ch = now->ch;
        while (ch)
        {
            if (dynamic_cast<folder_control_block*>(ch))
                printf("d");
            else printf("-");
            printf("%c%c%c. ", ch->get_rwx() & R ? 'r' : '-',
                   ch->get_rwx() & W ? 'w' : '-', ch->get_rwx() & X ? 'x' : '-');
            printf("%s ", ch->get_name());

            time_t ctime = ch->get_ctime(), mtime = ch->get_mtime();
            tm c_tm = *std::localtime(&ctime); tm m_tm = *std::localtime(&mtime);
            static char c_buf[50], m_buf[50];
            std::strftime(c_buf, sizeof(c_buf), "%Y-%m-%d %H:%M:%S", &c_tm);
            std::strftime(m_buf, sizeof(m_buf), "%Y-%m-%d %H:%M:%S", &m_tm);
            printf("ctime: %s | mtime: %s", c_buf, m_buf);
            printf("\n");
            ch = ch->sibling;
        }
        printf("\n");
    }

    bool create(const char* name)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        if (check_name(name))
        {
            fprintf(stderr, "error: file/folder %s exists.\n", name);
            return false;
        }

        int size = 1;
        int pfile = mem->apply(1);

        if (pfile == -1)
        {
            fprintf(stderr, "error: no available space.\n");
            return false;
        }

        MEMORY[pfile] = '\0';
        basic_block* tmp = now->ch;
        now->ch = new file_control_block(name, std::time(nullptr), 0777, size, pfile, now, tmp);
        now->modify_mtime(std::time(nullptr));
        return true;
    }

    bool write(const char* name, const char* data)
    {
        file_control_block* dst_file = find_file(name, now);
        if (!dst_file) return false;

        if (!(dst_file->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        int last_locate = dst_file->pfile;
        mem->free_by_locate(dst_file->pfile);
        int data_size = strlen(data);
        if (!data_size) data_size++;
        if ((dst_file->pfile = mem->apply(data_size)) == -1)
        {
            dst_file->pfile = mem->apply(dst_file->size);
            memcpy(MEMORY + dst_file->pfile, MEMORY + last_locate, dst_file->size * sizeof(char));
            fprintf(stderr, "error: no available space.(file has been recovered)\n");
            return false;
        }
        dst_file->size = data_size;
        memcpy(MEMORY + dst_file->pfile, data, data_size * sizeof(char));
        dst_file->modify_mtime(std::time(nullptr));
        now->modify_mtime(dst_file->get_mtime());
        return true;
    }

    bool read(const char* name)
    {
        file_control_block* dst_file = find_file(name, now);
        if (!dst_file) return false;

        if (!(dst_file->get_rwx() & R))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        char* out_str = MEMORY + dst_file->pfile;
        fwrite(out_str, 1, dst_file->size, stdout);
        printf("\n");
        return true;
    }

    bool mkdir(const char* name)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }
        if (check_name(name))
        {
            fprintf(stderr, "error: file/folder %s exists.\n", name);
            return false;
        }
        basic_block* tmp = now->ch;
        now->ch = new folder_control_block(name, std::time(nullptr), 0777, now, tmp);
        now->modify_mtime(std::time(nullptr));
        return true;
    }

    bool delete_file(const char* name)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }
        if (!check_name(name))
        {
            fprintf(stderr, "error: no such file.\n");
            return false;
        }
        file_control_block* dst_file = nullptr;
        if ((dst_file = find_file(name, now)) == nullptr)
        {
            fprintf(stderr, "tip: use \"deldir <foldername>\" instead.\n", name);
            return false;
        }

        basic_block* prev_block = find_prev(dst_file, now);
        if (!prev_block)
            now->ch = dst_file->sibling;
        else
            prev_block->sibling = dst_file->sibling;

        now->modify_mtime(std::time(nullptr));
        delete dst_file;
        return true;
    }

    bool delete_folder(const char* name)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }
        if (!check_name(name))
        {
            fprintf(stderr, "error: no such folder.\n");
            return false;
        }
        folder_control_block* dst_folder = nullptr;
        if ((dst_folder = find_folder(name, now)) == nullptr)
        {
            fprintf(stderr, "tip: use \"delete <filename>\" instead.\n", name);
            return false;
        }

        basic_block* prev_block = find_prev(dst_folder, now);
        if (!prev_block)
            now->ch = dst_folder->sibling;
        else
            prev_block->sibling = dst_folder->sibling;

        delete dst_folder;
        return true;
    }

    bool append(const char* name, const char* append_data)
    {
        file_control_block* dst_file = find_file(name, now);
        if (!dst_file) return false;
        if (!(dst_file->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        int last_locate = dst_file->pfile;
        mem->free_by_locate(dst_file->pfile);

        int len_append = strlen(append_data);
        int data_size = dst_file->size + len_append;
        if ((dst_file->pfile = mem->apply(data_size)) == -1)
        {
            dst_file->pfile = mem->apply(dst_file->size);
            memcpy(MEMORY + dst_file->pfile, MEMORY + last_locate, dst_file->size * sizeof(char));
            fprintf(stderr, "error: no available space.(file has been recovered)\n");
            return false;
        }
        char* data = new char[data_size];
        memcpy(data, MEMORY + last_locate, dst_file->size * sizeof(char));
        memcpy(data + dst_file->size, append_data, len_append * sizeof(char));

        dst_file->size = data_size;
        memcpy(MEMORY + dst_file->pfile, data, data_size * sizeof(char));
        dst_file->modify_mtime(std::time(nullptr));
        now->modify_mtime(dst_file->get_mtime());

        delete[] data;
        return true;
    }

    bool cp(const char* src, const char* dst)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        file_control_block* src_file = find_file(src, now);
        if (!src_file) return false;
        if (!(src_file->get_rwx() & R))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }

        if (check_name(dst))
        {
            fprintf(stderr, "error: duplicate file/folder name.\n");
            return false;
        }

        create(dst);
        char *tmp_s = new char[src_file->size];
        memcpy(tmp_s, MEMORY + src_file->pfile, src_file->size * sizeof(char));
        bool res = write(dst, tmp_s);
        delete[] tmp_s;
        return res;
    }

    bool rename(const char* old_name, const char* new_name)
    {
        if (!(now->get_rwx() & W))
        {
            fprintf(stderr, "Permission denied.\n");
            return false;
        }
        if (!check_name(old_name))
        {
            fprintf(stderr, "error: no such file/folder.\n");
            return false;
        }
        if (check_name(new_name))
        {
            fprintf(stderr, "error: duplicate file/folder name.\n");
            return false;
        }
        basic_block* ch = now->ch;
        while (ch)
        {
            if (!strcmp(ch->get_name(), old_name))
            {
                ch->modify_name(new_name);
                ch->modify_mtime(std::time(nullptr));
                now->modify_mtime(ch->get_mtime());
                return true;
            }
            ch = ch->sibling;
        }
        return false;
    }

    bool chmod(const char* name, const int& _rwx)
    {
        if (!check_name(name))
        {
            fprintf(stderr, "error: no such file/folder.\n");
            return false;
        }
        basic_block* ch = now->ch;
        while (ch)
        {
            if (!strcmp(ch->get_name(), name))
            {
                ch->modify_rwx(_rwx);
                return true;
            }
            ch = ch->sibling;
        }
        return false;
    }

    bool cd(const char* name)
    {
        if (!strcmp(name, ".")) return true;
        if (!strcmp(name, ".."))
        {
            if (now->parent) now = now->parent;
            return true;
        }

        folder_control_block* dst_folder = nullptr;
        if ((dst_folder = find_folder(name, now)) == nullptr)
            return false;

        if (dst_folder->get_rwx() & X)
        {
            now = dst_folder;
            printf("dir: %s\n", now->get_name());
            return true;
        }
        fprintf(stderr, "Permission denied.\n");
        return false;
    }

    void show_all()
    {
        mem->show();
        show_tree(&root_folder, 0);
    }
};

#endif /* _FILE_SIMULATOR_H_ */