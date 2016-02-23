/*
    header for TerrainEd
*/


#define FLOOR_SOLID   0
#define FLOOR_WATER   1
#define FLOOR_LAVA    2
#define FLOOR_NUKAGE  3
#define FLOOR_BLOOD   4
#define FLOOR_SLIME   5


typedef struct terraintype_s
{
    char     name[9];
    short    type;

} terraintype_t;

typedef struct queueitem_s
{
    struct queueitem_s    *next;
    terraintype_t         *terraintype;

} queueitem_t;


void ReadScript(FILE *file);
void WriteLump(FILE *file);
void StripSpaces(char *str);
void StripLSpace(char *str);
void StripRSpace(char *str);
void enqueue(terraintype_t *tt);

short TerrainTypeForName(char *type);

queueitem_t *dequeue(void);
