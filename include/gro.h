#ifndef GRO_H
# define GRO_H

# include <stdio.h>

typedef struct s_vec3 {
    double x;
    double y;
    double z;
} t_vec3;

typedef enum e_kind {
    KIND_OTHER = 0,
    KIND_SOLVENT = 1,
    KIND_ION = 2
} t_kind;

typedef struct s_molset {
    char   name[6];
    int    nmol;
    int    natoms;
    int   *idx;
    t_kind kind;
} t_molset;

typedef struct s_bounds {
    t_vec3 lo;
    t_vec3 hi;
} t_bounds;

typedef struct s_channel {
    t_vec3 lo;
    t_vec3 hi;
    int    axis;
} t_channel;

typedef struct s_summary {
    t_molset *sets;
    int       nsets;
    int       cap;
    t_channel chan;
    t_bounds  local_box;
} t_summary;

typedef struct s_topo {
    int     natoms;
    t_vec3  box[3];
    t_vec3 *x;
    int start_mol;
    int count_mol;
    t_summary sum;
} t_topo;

int  gro_parse(FILE *fp, t_topo *f0);
void gro_free(t_topo *f0);

void gro_channel_update(const t_topo *f0);
int  gro_channel_load(t_topo *f0, const char *path);
int  gro_channel_save(const t_topo *f0, const char *path);
int  gro_channel_update_or_load(t_topo *f0, const char *path);

void gro_print_essentials(const t_topo *f0);

#endif /* GRO_H */
