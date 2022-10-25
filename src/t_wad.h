#ifndef T_WAD_H
#define T_WAD_H

//
// TYPES
//
typedef struct
{
    // Should be "IWAD" or "PWAD".
    char		identification[4];
    int			numlumps;
    int			infotableofs;

} wadinfo_t;


typedef struct
{
    int			filepos;
    int			size;
    char		name[8];

} filelump_t;

typedef struct
{
    int			filepos;
    int			size;
	int			compressedsize;
    char		name[8];

} cfilelump_t;

//
// WADFILE I/O related stuff.
//
typedef struct
{
    char	name[8];
    int		handle;
    int		position;
    int		size;
	int		compressedsize;
} lumpinfo_t;


#endif