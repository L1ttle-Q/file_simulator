/*mem_simulator.cpp

author: L1ttle-Q
date: 2025-5-10
upd: 2025-6-8

memory simulator
strategy:
first fit
best fit
next fit
worst fit

operation:
show
alloc
free
exit

memory merge
memory defragmentation

structure:
linked list
priority queue
*/

#ifndef _MEM_SIMULATOR_H_
#define _MEM_SIMULATOR_H_

#include <queue>
#include <cstdio>
#include <vector>
#include <cstring>
using std::priority_queue;
using std::less;
using std::greater;
using std::vector;

const int SEGMENT_MAX = 1024;
const int MEMORY_STORAGY = 32768;

enum Strategy
{
    first_fit,
    best_fit,
    next_fit,
    worst_fit
};
extern Strategy strategy;
extern bool Mem_op_print;

namespace mem_simulator_operation
{
    enum Operation
    {
        Show,
        Alloc,
        Free,
        Exit
    };
}

class Memory_simulator
{
private:
    struct Segment
    {
        int id;
        int pid;
        int first;
        int end;
        int status; // 0 for free; 1 for allocated
        int last_modify;

        Segment(): id(0), pid(0), first(0), end(0), status(0), last_modify(0) {}
        Segment(int Id, int Pid, int First, int End, int Status, int Last_modify):
            id(Id), pid(Pid), first(First), end(End), status(Status), last_modify(Last_modify) {}
        int size() const {return end - first + 1;}

        bool operator < (const Segment &b) const {return size() < b.size();}
        bool operator > (const Segment &b) const {return size() > b.size();}
    };

    struct Segment_List
    {
        Segment content;
        Segment_List* prev;
        Segment_List* next;

        Segment_List(): content(), prev(nullptr), next(nullptr){}
    } segment_head;

    priority_queue < Segment, vector<Segment>, less<Segment> > Max_heap;
    priority_queue < Segment, vector<Segment>, greater<Segment> > Min_heap;

    int next_locate;
    int segment_cnt;
    int Modify[SEGMENT_MAX];

    Segment_List* locate_segment(const int& locate, const Segment_List& begin)
    {
        if (locate < 0 || locate >= SEGMENT_MAX) return NULL;
        Segment_List* now = begin.next;
        while (now)
        {
            if (now->content.end < locate)
            {
                now = now->next;
                continue;
            }
            return now;
        }
        return NULL;
    }

    bool could_allocate(const int& size)
    {
        Segment now;
        while (!Max_heap.empty())
        {
            now = Max_heap.top(); Max_heap.pop();
            if (now.last_modify < Modify[now.id]) continue;
            Max_heap.push(now);
            if (now.size() < size)
                return false;
            return true;
        }
        return false;
    }

    int decide_memory(const int& size)
    {
        static vector<Segment> temp_segments;

        if (size <= 0 || !could_allocate(size)) return -1;

        Segment_List* pnow;
        Segment now;
        switch (strategy)
        {
            case first_fit:
            pnow = segment_head.next;
            while (pnow)
            {
                if (pnow->content.status || pnow->content.size() < size)
                {
                    pnow = pnow->next;
                    continue;
                }
                return pnow->content.first;
            }
            break;

            case best_fit:
            while (!Min_heap.empty())
            {
                now = Min_heap.top(); Min_heap.pop();
                if (now.last_modify < Modify[now.id]) continue;
                temp_segments.push_back(now);
                if (now.size() >= size)
                {
                    for (const auto& seg : temp_segments)
                        Min_heap.push(seg);
                    temp_segments.clear();
                    return now.first;
                }
            }
            for (const auto& seg : temp_segments)
                Min_heap.push(seg);
            temp_segments.clear();
            break;

            case next_fit:
            pnow = locate_segment(next_locate, segment_head);
            while (pnow)
            {
                if (pnow->content.status || pnow->content.size() < size)
                {
                    pnow = pnow->next;
                    if (!pnow) pnow = segment_head.next;
                    continue;
                }
                return pnow->content.first;
            }
            break;

            case worst_fit:
            while (!Max_heap.empty())
            {
                now = Max_heap.top(); Max_heap.pop();
                if (now.last_modify < Modify[now.id]) continue;
                Max_heap.push(now);
                if (now.size() < size)
                    return -1;
                return now.first;
            }
            break;
        }
        return -1;
    }

public:
    Memory_simulator(): segment_cnt(0), next_locate(0), segment_head()
    {
        Segment_List* new_node = new Segment_List();
        segment_head.next = new_node;
        new_node->prev = &segment_head;
        new_node->content = Segment(segment_cnt++, 0, 0, MEMORY_STORAGY - 1, 0, ++Modify[0]);
        Max_heap.push(new_node->content);
        Min_heap.push(new_node->content);
        memset(Modify, 0, sizeof(Modify));
    }

    int apply(const int& size) // return applied segment first place; fail for -1
    {
        int decide = decide_memory(size);
        if (!~decide)
        {
            fprintf(stderr, "error: cannot alloc.\n");
            return -1;
        }

        Segment_List* new_seg = new Segment_List();
        Segment& now = new_seg->content;
        now.id = segment_cnt++;
        now.first = decide;
        now.end = decide + size - 1;
        now.status = 1;
        now.last_modify = ++Modify[now.id];

        next_locate = (now.end + 1) % SEGMENT_MAX;

        Segment_List* last_seg = locate_segment(decide, segment_head);
        Modify[last_seg->content.id]++;
        last_seg->prev->next = new_seg;
        new_seg->prev = last_seg->prev;

        Segment_List* next_seg = last_seg->next;
        if (last_seg->content.size() > size)
        {
            next_seg = new Segment_List();
            next_seg->content = Segment(last_seg->content.id, 0, new_seg->content.end + 1, last_seg->content.end, 0, ++Modify[last_seg->content.id]);
            next_seg->next = last_seg->next;
            if (last_seg->next) last_seg->next->prev = next_seg;
            Max_heap.push(next_seg->content);
            Min_heap.push(next_seg->content);
        }
        new_seg->next = next_seg;
        if (next_seg) next_seg->prev = new_seg;
        delete last_seg;

        if (Mem_op_print)
            printf("alloc: id: %d, range[%d, %d]\n", now.id, now.first, now.end);

        return decide;
    }

    bool free_by_locate(const int& locate)
    {
        int id = get_id(locate);
        if (id == -1) return false;
        return free(id);
    }

    int get_id(const int& locate)
    {
        if (locate < 0 || locate >= MEMORY_STORAGY)
            return -1;

        Segment_List* now = segment_head.next;
        Segment* dst_seg = NULL;

        while (now)
        {
            if (now->content.first <= locate && now->content.end >= locate)
            {
                dst_seg = &(now->content);
                break;
            }
            now = now->next;
        }
        if (!dst_seg)
        {
            fprintf(stderr, "error: wrong location.\n");
            return -1;
        }
        return dst_seg->id;
    }

    bool free(const int& id)
    {
        if (id <= 0 || segment_cnt <= id)
        {
            fprintf(stderr, "error: wrong id number.\n");
            return false;
        }
        Segment_List* now = segment_head.next;
        Segment* dst_seg = NULL;
        while (now)
        {
            if (now->content.id == id)
            {
                dst_seg = &(now->content);
                break;
            }
            now = now->next;
        }
        if (!dst_seg || !dst_seg->status)
        {
            fprintf(stderr, "error: cannot free.\n");
            return false;
        }

        dst_seg->id = segment_cnt++;
        dst_seg->status = 0;
        dst_seg->last_modify = ++Modify[dst_seg->id];

        if (Mem_op_print)
            printf("free: id: %d, range[%d, %d] size: %d\n",
               dst_seg->id, dst_seg->first, dst_seg->end, dst_seg->size());

        // merge
        if (now->prev != &segment_head)
        {
            if (!now->prev->content.status)
            {
                Segment_List* tmp = now->prev;
                if (Mem_op_print)
                    printf("merge: %d[%d, %d] %d[%d, %d]\n",
                        tmp->content.id, tmp->content.first, tmp->content.end, now->content.id, now->content.first, now->content.end);
                dst_seg->first = tmp->content.first;
                Modify[tmp->content.id]++;
                tmp->prev->next = now;
                now->prev = tmp->prev;
                delete tmp;
            }
        }
        if (now->next)
        {
            if (!now->next->content.status)
            {
                Segment_List* tmp = now->next;
                if (Mem_op_print)
                    printf("merge: %d[%d, %d] %d[%d, %d]\n",
                        now->content.id, now->content.first, now->content.end, tmp->content.id, tmp->content.first, tmp->content.end);
                dst_seg->end = tmp->content.end;
                Modify[tmp->content.id]++;
                if (tmp->next) tmp->next->prev = now;
                now->next = tmp->next;
                delete tmp;
            }
        }

        Max_heap.push(*dst_seg);
        Min_heap.push(*dst_seg);
        return true;
    }

    void show()
    {
        printf("Memory Assignment:\n");
        Segment_List* now = segment_head.next;
        while (now)
        {
            printf("segment %d: [%d, %d], size: %d, status: %s\n",
                   now->content.id, now->content.first, now->content.end, now->content.size(), now->content.status ? "allocated" : "free");
            now = now->next;
        }
    }

    void defragment()
    {}
};

#endif /* _MEM_SIMULATOR_H_ */