#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ---------- Model ----------
typedef struct {
    int    id;
    time_t due;                  // 0 = no due
    char   description[256];
} Task;

typedef struct Node {
    Task        task;
    struct Node *next;
} Node;

// ---------- Globals ----------
static Node *head = NULL;           // active
static Node *trash_head = NULL;     // removed
static int   nextId = 1;

static char  active_file[512]  = "tasks.txt";
static char  removed_file[512] = "removed.txt";

// ---------- Minimal ANSI color ----------
static bool use_ansi(void) {
    const char *u = getenv("USE_COLOR");
    return !u || strcmp(u, "0") != 0;
}
static const char *S_RESET(void){ return use_ansi() ? "\x1b[0m"  : ""; }
static const char *S_BOLD(void) { return use_ansi() ? "\x1b[1m"  : ""; }
static const char *C_CYAN(void) { return use_ansi() ? "\x1b[36m" : ""; }
static const char *C_YELL(void) { return use_ansi() ? "\x1b[33m" : ""; }
static const char *C_GRN(void)  { return use_ansi() ? "\x1b[32m" : ""; }
static const char *C_RED(void)  { return use_ansi() ? "\x1b[31m" : ""; }

// ---------- Utils ----------
static int parseInt(const char *s, int *out) {
    if (!s || !*s) return -1;
    char *end = NULL; long v = strtol(s, &end, 10);
    if (*end != '\0') return -1;
    *out = (int)v; return 0;
}
static char *lcase(char *s) { for (char *p=s; *p; ++p) *p=(char)tolower((unsigned char)*p); return s; }

static void init_paths(void) {
    const char *a = getenv("CLITASK_FILE");
    const char *r = getenv("CLITASK_REMOVED");
    if (a && *a) { strncpy(active_file, a, sizeof active_file - 1); active_file[sizeof active_file - 1] = '\0'; }
    if (r && *r) { strncpy(removed_file, r, sizeof removed_file - 1); removed_file[sizeof removed_file - 1] = '\0'; }
}

// ---------- Dates & times ----------
static bool parse_mmdd(const char *tok, int *m, int *d) {
    int mm=0, dd=0; if (sscanf(tok, "%d/%d", &mm, &dd)==2) {
        if (mm>=1 && mm<=12 && dd>=1 && dd<=31) { *m=mm; *d=dd; return true; }
    } return false;
}
static bool parse_time_token(const char *tok, int *out_h, int *out_min) {
    char s[32]; strncpy(s, tok, sizeof s - 1); s[sizeof s - 1]='\0'; lcase(s);
    int h=0, m=0; bool pm=false, am=false;
    size_t L=strlen(s);
    if (L>=2 && strcmp(s+L-2,"pm")==0){ pm=true; s[L-2]='\0'; }
    else if (L>=2 && strcmp(s+L-2,"am")==0){ am=true; s[L-2]='\0'; }
    if (sscanf(s, "%d:%d", &h, &m) != 2) { if (sscanf(s, "%d", &h)!=1) return false; m=0; }
    if (pm||am) { if (h<1||h>12) return false; if (pm && h!=12) h+=12; if (am && h==12) h=0; }
    if (h<0||h>23||m<0||m>59) return false;
    *out_h=h; *out_min=m; return true;
}
static time_t make_time_local(int Y,int M,int D,int h,int m){
    struct tm tm={0}; tm.tm_year=Y-1900; tm.tm_mon=M-1; tm.tm_mday=D; tm.tm_hour=h; tm.tm_min=m; tm.tm_isdst=-1;
    return mktime(&tm);
}
static void today_YMD(int *Y,int *M,int *D,int *w){
    time_t now=time(NULL); struct tm tm; localtime_r(&now,&tm);
    if(Y)*Y=tm.tm_year+1900; if(M)*M=tm.tm_mon+1; if(D)*D=tm.tm_mday; if(w)*w=tm.tm_wday;
}
static bool same_ymd(time_t a, time_t b){
    if(!a||!b) return false;
    struct tm ta,tb; localtime_r(&a,&ta); localtime_r(&b,&tb);
    return ta.tm_year==tb.tm_year && ta.tm_mon==tb.tm_mon && ta.tm_mday==tb.tm_mday;
}
static void fmt_when(time_t t, char *out, size_t L) {
    if (!t) { snprintf(out,L,"-"); return; }
    struct tm tm; localtime_r(&t,&tm);
    strftime(out,L,"%Y-%m-%d %H:%M",&tm);
}
static time_t parse_due(const char *date_tok, const char *time_tok) {
    int Y,M,D,w; today_YMD(&Y,&M,&D,&w);
    int outM=M, outD=D; bool have_date=false;

    if (date_tok && *date_tok) {
        char buf[32]; strncpy(buf,date_tok,sizeof buf - 1); buf[sizeof buf - 1]='\0'; lcase(buf);
        if (strcmp(buf,"today")==0) { have_date=true; }
        else if (strcmp(buf,"tomorrow")==0) {
            time_t t=make_time_local(Y,M,D,0,0)+24*3600; struct tm tm; localtime_r(&t,&tm);
            Y=tm.tm_year+1900; outM=tm.tm_mon+1; outD=tm.tm_mday; have_date=true;
        } else if (parse_mmdd(buf,&outM,&outD)) {
            have_date=true;
            time_t cand=make_time_local(Y,outM,outD,23,59);
            if (difftime(cand,time(NULL))<0) Y+=1;
        }
    }

    int hh=23, mm=59;
    if (time_tok && *time_tok) {
        if (!parse_time_token(time_tok,&hh,&mm)) return 0;
    } else if (!have_date) {
        return 0; // no due
    }
    return make_time_local(Y,outM,outD,hh,mm);
}

// ---------- Linked list helpers ----------
static void list_push_head(Node **h, Task t){
    Node *n=(Node*)malloc(sizeof *n); if(!n){perror("malloc"); exit(1);}
    n->task=t; n->next=*h; *h=n;
}
static bool list_remove_by_id(Node **h,int id, Task *out){
    Node *cur=*h,*prev=NULL;
    while(cur){ if(cur->task.id==id){ if(prev)prev->next=cur->next; else *h=cur->next; if(out)*out=cur->task; free(cur); return true; }
                 prev=cur; cur=cur->next; }
    return false;
}
static size_t list_count(Node *h){ size_t c=0; for(;h;h=h->next) c++; return c; }
static void list_free(Node **h){ Node *c=*h; while(c){Node *n=c; c=c->next; free(n);} *h=NULL; }

// ---------- Storage ----------
static bool load_file(const char *path, Node **out_head, int *io_nextId) {
    *out_head = NULL; Node *tail=NULL;
    FILE *f=fopen(path,"r"); if(!f) return true;
    char line[1024];
    while (fgets(line,sizeof line,f)) {
        int id=0; long long due=0; char desc[256]={0};
        int n = sscanf(line,"%d %lld %255[^\n]", &id,&due,desc);
        if (n < 3) continue;
        Task t={0}; t.id=id; t.due=(time_t)due;
        strncpy(t.description,desc,sizeof t.description - 1);

        Node *node=(Node*)malloc(sizeof *node); if(!node){perror("malloc"); fclose(f); exit(1);}
        node->task=t; node->next=NULL;
        if(!*out_head) { *out_head=node; tail=node; } else { tail->next=node; tail=node; }
        if (io_nextId && id >= *io_nextId) *io_nextId = id + 1;
    }
    fclose(f); return true;
}
static bool save_file(const char *path, Node *h, bool verbose){
    FILE *f=fopen(path,"w"); if(!f){perror("open for write"); return false;}
    for(Node *n=h;n;n=n->next){
        fprintf(f,"%d %lld %s\n", n->task.id, (long long)n->task.due, n->task.description);
    }
    fclose(f); if(verbose) printf("Saved %s\n", path); return true;
}
static void save_all_quiet(void){ save_file(active_file, head, false); save_file(removed_file, trash_head, false); }
static void save_all_verbose(void){ save_file(active_file, head, true); save_file(removed_file, trash_head, true); }

// ---------- Formatting ----------
static void print_welcome_header(void){
    const char *B=S_BOLD(), *R=S_RESET(), *CY=C_CYAN();
    printf("%s%s========================================%s\n", CY,B,R);
    printf("%s  Welcome to CLI Task Manager%s\n", B,R);
    printf("%s========================================%s\n", CY,B?R:"");
    time_t now=time(NULL); char buf[64]; struct tm tm; localtime_r(&now,&tm);
    strftime(buf,sizeof buf,"%A, %Y-%m-%d %H:%M",&tm);
    printf("Now: %s\n", buf);
    printf("Active:  %s\n", active_file);
    printf("Removed: %s\n\n", removed_file);
}

static bool is_today_local(time_t t){
    if(!t) return false; time_t now=time(NULL); return same_ymd(t, now);
}
static bool is_tomorrow_local(time_t t){
    if (!t) return false;
    time_t now=time(NULL);
    struct tm tm; localtime_r(&now,&tm);
    tm.tm_mday += 1;
    time_t tom = mktime(&tm);
    return same_ymd(t, tom);
}

static int cmp_task_ptrs(const void *a,const void *b){
    const Node *na=*(const Node * const *)a, *nb=*(const Node * const *)b;
    if (na->task.due==0 && nb->task.due==0) return na->task.id - nb->task.id;
    if (na->task.due==0) return 1;
    if (nb->task.due==0) return -1;
    if (na->task.due < nb->task.due) return -1;
    if (na->task.due > nb->task.due) return 1;
    return na->task.id - nb->task.id;
}
static void print_task_row(const Task *t){
    char when[32]; fmt_when(t->due, when, sizeof when);
    printf("%-16s  %-3d %s\n", when, t->id, t->description);
}

// ---------- Commands ----------
static void cmd_help(int argc, char **argv){
    (void)argc; (void)argv;
    print_welcome_header();
    printf("Commands:\n");
    printf("  add \"desc\" [date] [time]\n");
    printf("  list\n");
    printf("  delete <id>\n");
    printf("  removed\n");
    printf("  save\n");
    printf("  help\n");
    printf("  checklist\n\n");
}
static void cmd_add(int argc, char **argv){
    if (argc < 1) { printf("Usage: add \"desc\" [date] [time]\n"); return; }
    const char *desc_in = argv[0];
    const char *date_tok = (argc>=2)? argv[1] : NULL;
    const char *time_tok = (argc>=3)? argv[2] : NULL;

    char desc[256]; strncpy(desc, desc_in, sizeof desc - 1); desc[sizeof desc - 1]='\0';
    time_t due = parse_due(date_tok, time_tok);

    Task t={0}; t.id=nextId++; t.due=due;
    strncpy(t.description, desc, sizeof t.description - 1);

    list_push_head(&head, t);
    char when[32]; fmt_when(t.due, when, sizeof when);
    printf("%sAdded%s #%d: %s  (due: %s)\n", C_GRN(), S_RESET(), t.id, t.description, when);
    save_all_quiet();
}
static void cmd_list(int argc, char **argv){
    (void)argc; (void)argv;
    print_welcome_header();
    size_t n=list_count(head);
    if (n==0) { printf("No tasks.\n"); return; }
    Node **arr=(Node**)malloc(n * sizeof *arr); if(!arr){perror("malloc"); return;}
    size_t i=0; for(Node *c=head;c;c=c->next) arr[i++]=c;
    qsort(arr,n,sizeof(Node*),cmp_task_ptrs);

    printf("%sToday's Tasks%s\n", C_GRN(), S_RESET());
    printf("Due               ID  Description\n");
    printf("----------------  --  ------------------------------\n");
    size_t printed_today=0;
    for (i=0;i<n;i++) {
        const Task *t=&arr[i]->task;
        if (t->due && is_today_local(t->due)) { print_task_row(t); printed_today++; }
    }
    if (!printed_today) printf("  (none)\n");
    printf("\n");

    printf("%sTomorrow's Tasks%s\n", C_YELL(), S_RESET());
    printf("Due               ID  Description\n");
    printf("----------------  --  ------------------------------\n");
    size_t printed_tom=0;
    for (i=0;i<n;i++) {
        const Task *t=&arr[i]->task;
        if (t->due && is_tomorrow_local(t->due)) { print_task_row(t); printed_tom++; }
    }
    if (!printed_tom) printf("  (none)\n");
    printf("\n");

    printf("%sAll Tasks%s (sorted by due; undated last)\n", C_CYAN(), S_RESET());
    printf("Due               ID  Description\n");
    printf("----------------  --  ------------------------------\n");
    for (i=0;i<n;i++) print_task_row(&arr[i]->task);
    free(arr);
}
static void cmd_delete(int argc, char **argv){
    if (argc<1){ printf("Usage: delete <id>\n"); return; }
    int id=0; if(parseInt(argv[0],&id)!=0){ printf("Invalid id.\n"); return; }
    Task t; if(!list_remove_by_id(&head,id,&t)){ printf("Task %d not found.\n", id); return; }
    list_push_head(&trash_head,t);
    printf("%sRemoved%s #%d.\n", C_RED(), S_RESET(), id);
    save_all_quiet();
}
static void cmd_removed(int argc, char **argv){
    (void)argc; (void)argv;
    if(!trash_head){ printf("Removed is empty.\n"); return; }
    printf("%sRemoved Tasks%s\n", C_RED(), S_RESET());
    printf("Due               ID  Description\n");
    printf("----------------  --  ------------------------------\n");
    for(Node *n=trash_head;n;n=n->next){
        char when[32]; fmt_when(n->task.due, when, sizeof when);
        printf("%-16s  %-3d %s\n", when, n->task.id, n->task.description);
    }
}
static void cmd_save(int argc, char **argv){ (void)argc; (void)argv; save_all_verbose(); }
static void cmd_checklist(int argc, char **argv){
    (void)argc; (void)argv;
    printf("The Thinker's Checklist:\n");
    printf("1. Invert, always invert.\n");
    printf("2. The joy and power of compounding.\n");
    printf("3. Multidisciplinary thinking avoids man-with-the-hammer-syndrome.\n");
    printf("4. Deleting ignorance is a moral duty.\n");
    printf("5. Taking care of your family is a moral duty.\n");
    printf("6. Defer gratification.\n");
    printf("7. The best way to get what you want is to deserve what you want.\n");
    printf("8. Read.\n");
    printf("9. Be rational.\n");
    printf("10. All I want to know is where I will die so I don't go there.\n");
    printf("11. There is great advantage in trying to be less stupid than others rather than trying to outsmart others.\n");
    printf("12. Stay cheerful despite your troubles.\n");
    printf("13. Don't have resentment, don't have envy.\n");
    printf("14. Do what you are supposed to do.\n");
    printf("15. Deal with reliable people.\n");
    printf("16. Master the best of what other people have already figured out to gain wisdom.\n");
    printf("17. Only sell what you would buy if you were on the other end.\n");
    printf("18. Befriending the wise eminent dead can be helpful.\n");
    printf("19. Have low expectations.\n");
    printf("20. It is asinine to risk what you need for what you desire.\n");
    printf("21. Work, work, work, and hope to have a few good insights.\n");
    printf("22. Those who continue to learn will continue to rise in life.\n");
}

// ---------- Dispatch ----------
typedef void (*handler_t)(int,char**);
typedef struct { const char *name; handler_t fn; } Command;

static Command CMDS[] = {
    {"add",     cmd_add},
    {"list",    cmd_list},
    {"remove",  cmd_delete},
    {"removed", cmd_removed},
    {"save",    cmd_save},
    {"help",    cmd_help},
    {"checklist", cmd_checklist},
    {NULL,      NULL}
};

static void load_all(void){
    load_file(active_file,&head,&nextId);
    load_file(removed_file,&trash_head,&nextId);
}
static void at_exit_cleanup(void){
    save_all_quiet();
    list_free(&head); list_free(&trash_head);
}

int main(int argc, char **argv){
    atexit(at_exit_cleanup);
    init_paths();
    load_all();

    if (argc<2){ cmd_help(0,NULL); return 2; }
    const char *cmd=argv[1];
    for (int i=0; CMDS[i].name; ++i){
        if (strcmp(CMDS[i].name, cmd)==0) { CMDS[i].fn(argc-2, argv+2); return 0; }
    }
    printf("Unknown command: %s\nTry: %s help\n", cmd, argv[0]);
    return 2;
}

