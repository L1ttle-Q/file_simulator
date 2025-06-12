/*file_save.cpp

author: L1ttle-Q
date: 2025-6-8
upd: 2025-6-9

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

#include <cstring>
#include "file_save.h"
#include "file_simulator.h"

const char reserved[] = {';', '[', ']', '(', ')', '\"', '\\', '{', '}'};
constexpr int Reserved_size = sizeof(reserved) / sizeof(char);

extern FILE* FILE_ISTREAM;
extern FILE* FILE_OSTREAM;

static const int FILE_BUFFER_MAX = 1024;
static char FILE_BUFFER[FILE_BUFFER_MAX];
static char *now = FILE_BUFFER, *end = FILE_BUFFER;
char getNextChar()
{
    if (now == end)
    {
        end = (now = FILE_BUFFER) + fread(FILE_BUFFER, 1, FILE_BUFFER_MAX, FILE_ISTREAM);
        if (now == end) return EOF;
    }
    return *now;
}
void move_ahead() {now++;}
#define match(c) do { if (!_match(c)) return false; } while(0)
bool _match(const char& c)
{
    if (now == end)
    {
        end = (now = FILE_BUFFER) + fread(FILE_BUFFER, 1, FILE_BUFFER_MAX, FILE_ISTREAM);
        if (now == end) return false;
    }
    if (*now == c)
    {
        now++;
        return true;
    }
    fprintf(stderr, "not match: expect %c, but get %c\n", c, *now);
    return false;
}

bool Reserved(const char& c)
{
    for (int i = 0; i < Reserved_size; i++)
        if (c == reserved[i])
            return true;
    return false;
}
bool Reserved(const char* s)
{
    int len = strlen(s);
    for (int i = 0; i < len; i++)
        for (int j = 0; j < Reserved_size; j++)
            if (s[i] == reserved[j])
                return true;
    return false;
}

bool valid_ch(const char& c)
{
    return !Reserved(c);
}

void File_simulator_constructor::SetFolderAttr(File_simulator* p, const char* name, const time_t ctime, const time_t mtime, const int rwx)
{
    folder_control_block* cur = p->now;
    if (cur == &p->root_folder && strcmp(name, "")) fprintf(stderr, "warning: root folder can not be renamed.(saved file has been changed)\n");
    else cur->modify_name(name);
    cur->modify_ctime(ctime);
    cur->modify_mtime(mtime);
    cur->modify_rwx(rwx);
}

void File_simulator_constructor::SetFileAttr(File_simulator* p, const char* name, const time_t ctime, const time_t mtime, const int rwx)
{
    file_control_block* p_file = p->find_file(name, p->now);
    p_file->modify_ctime(ctime);
    p_file->modify_mtime(mtime);
    p_file->modify_rwx(rwx);
}
void File_simulator_constructor::Setrwx(File_simulator *p, const int rwx)
{
    folder_control_block* cur = p->now;
    cur->modify_rwx(rwx);
}
void File_simulator_constructor::SaveSimulator(folder_control_block* cur)
{
    fprintf(FILE_OSTREAM, "[%s;%lld;%lld;%d]", cur->get_name(), cur->get_ctime(), cur->get_mtime(), cur->get_rwx());
    fprintf(FILE_OSTREAM, "{");
    basic_block* ch = cur->ch;
    while (ch)
    {
        if (dynamic_cast<folder_control_block*>(ch))
            SaveSimulator(dynamic_cast<folder_control_block*>(ch));
        else
        {
            file_control_block* p_file = dynamic_cast<file_control_block*>(ch);
            fprintf(FILE_OSTREAM, "(%s;%lld;%lld;%d)", ch->get_name(), ch->get_ctime(), ch->get_mtime(), ch->get_rwx());
            fprintf(FILE_OSTREAM, "\"");
            char* tmp_c = File_simulator::MEMORY + p_file->pfile;
            for (int i = 0; i < p_file->size; i++)
            {
                if (Reserved(*tmp_c)) fprintf(FILE_OSTREAM, "\\");
                fprintf(FILE_OSTREAM, "%c", *tmp_c++);
            }
            fprintf(FILE_OSTREAM, "\"");
        }
        ch = ch->sibling;
    }
    fprintf(FILE_OSTREAM, "}");
}
void File_simulator_constructor::SaveSimulator(File_simulator *p)
{
    folder_control_block* cur = &p->root_folder;
    SaveSimulator(cur);
}

File_simulator_constructor constructor;

void SaveSimulator(File_simulator* p)
{
    constructor.SaveSimulator(p);
}

bool fdcb(File_simulator* now, int& rwx)
{
    static char name[MAX_NAME_LENGTH];
    time_t mtime = 0, ctime = 0;
    rwx = 0;

    match('[');
    char c = getNextChar();
    int idx_name = 0;

    while (c != ';')
    {
        if (Reserved(c))
        {
            fprintf(stderr, "error: invalid saved file.(name with reserved character)\n");
            return false;
        }
        name[idx_name++] = c;
        move_ahead(); c = getNextChar();
    } name[idx_name++] = '\0'; match(';'); c = getNextChar();
    while (c != ';')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        ctime = (ctime << 3) + (ctime << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(';'); c = getNextChar();
    while (c != ';')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        mtime = (mtime << 3) + (mtime << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(';'); c = getNextChar();
    while (c != ']')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        rwx = (rwx << 3) + (rwx << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(']');
    constructor.SetFolderAttr(now, name, ctime, mtime, 0777);
    return true;
}

bool fcb(File_simulator* now, char* name, time_t& mtime, time_t& ctime, int& rwx)
{
    mtime = 0; ctime = 0;
    rwx = 0;

    match('(');
    char c = getNextChar();
    int idx_name = 0;

    while (c != ';')
    {
        if (Reserved(c))
        {
            fprintf(stderr, "error: invalid saved file.(name with reserved character)\n");
            return false;
        }
        name[idx_name++] = c;
        move_ahead(); c = getNextChar();
    } name[idx_name++] = '\0'; match(';'); c = getNextChar();
    while (c != ';')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        ctime = (ctime << 3) + (ctime << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(';'); c = getNextChar();
    while (c != ';')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        mtime = (mtime << 3) + (mtime << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(';'); c = getNextChar();
    while (c != ')')
    {
        if (!isdigit(c))
        {
            fprintf(stderr, "error: invalid saved file.(time with non-number character)\n");
            return false;
        }
        rwx = (rwx << 3) + (rwx << 1) + c - '0';
        move_ahead(); c = getNextChar();
    } match(')'); c = getNextChar();
    now->rename(";tmpfile", name);
    return true;
}

bool E(char*, int&);
bool C(char*, int&);
bool G(File_simulator*);
bool A(File_simulator*);
bool B(File_simulator*);

bool E(char* content, int& idx)
{
    char c = getNextChar();
    if (!Reserved(c)) {move_ahead(); content[idx++] = c;}
    else if (c == '\\') {move_ahead(); c = getNextChar(); move_ahead(); content[idx++] = c;}
    else
    {
        fprintf(stderr, "error: invalid saved file.(reserved character exists without \'\\\')\n");
        return false;
    }
    return true;
}

bool C(char* content, int& idx)
{
    char c = getNextChar();
    while (c != '\"')
    {
        if (!E(content, idx)) return false;
        c = getNextChar();
    }

    content[idx++] = '\0';
    return true;
}

bool S(File_simulator* now)
{
    int rwx;
    if (!fdcb(now, rwx)) return false;
    match('{');
    if (!G(now)) return false;
    match('}');
    constructor.Setrwx(now, rwx);
    if (getNextChar() != EOF) fprintf(stderr, "warning: spare character(s) at end.\n");
    return true;
}

bool G(File_simulator* now)
{
    char c = getNextChar();
    if (c == '[')
    {
        if (!A(now)) return false;
        return G(now);
    }
    else if (c == '(')
    {
        if (!B(now)) return false;
        return G(now);
    }
    else if (c == '}') return true;
    else
    {
        fprintf(stderr, "error: invalid saved file.(begin with unexpected character)\n");
        return false;
    }
}

bool A(File_simulator* now)
{
    int rwx;
    now->mkdir(";tmpdir"); // use unavailable character to avoid conflict
                        // in every menu, there is at most 1 tmp dir
    now->cd(";tmpdir");
    if (!fdcb(now, rwx)) return false;
    match('{');
    if (!G(now)) return false;
    match('}');
    constructor.Setrwx(now, rwx);
    now->cd("..");
    return true;
}

bool B(File_simulator* now)
{
    static char name[MAX_NAME_LENGTH];
    time_t ctime, mtime;
    int rwx;
    static char content[MEMORY_STORAGY];
    static int len_content;

    len_content = 0;
    now->create(";tmpfile");
    if (!fcb(now, name, ctime, mtime, rwx)) return false;
    match('\"');
    if (!C(content, len_content)) return false;
    match('\"');
    now->write(name, content);
    constructor.SetFileAttr(now, name, ctime, mtime, rwx);
    return true;
}
