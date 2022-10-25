//t_player.h


#ifndef T_PLAYER_H
#define T_PLAYER_H

#include "t_items.h"
#include "t_pspr.h"
#include "t_mobj.h"
#include "t_ticcmd.h"

//
// Player states.
//
typedef enum
{
    // Playing or camping.
    PST_LIVE,
    // Dead on the ground, view follows killer.
    PST_DEAD,
    // Ready to restart/respawn???
    PST_REBORN

} playerstate_t;


//
// Player internal flags, for cheats and debug.
//
typedef enum
{
    // No clipping, walk through barriers.
    CF_NOCLIP		= 1,
    // No damage, no health loss.
    CF_GODMODE		= 2,
    // Not really a cheat, just a debug aid.
    CF_NOMOMENTUM	= 4

} cheat_t;


//
// Extended player object info: player_t
//
typedef struct player_s
{
    mobj_t*		mo;
    playerstate_t	playerstate;
    ticcmd_t		cmd;

    // Determine POV,
    //  including viewpoint bobbing during movement.
    // Focal origin above r.z
    fixed_t		viewz;
    // Base height above floor for viewz.
    fixed_t		viewheight;
    // Bob/squat speed.
    fixed_t         	deltaviewheight;
    // bounded/scaled total momentum.
    fixed_t         	bob;

    // This is only used between levels,
    // mo->health is used during levels.
    int			health;
    int			armorpoints;
    // Armor type is 0-2.
    int			armortype;

    // Power ups. invinc and invis are tic counters.
    int			powers[NUMPOWERS];
    dboolean		cards[NUMCARDS];
    dboolean		backpack;

    // Frags, kills of other players.
    int			frags[MAXPLAYERS];
    weapontype_t	readyweapon;

    // Is wp_nochange if not changing.
    weapontype_t	pendingweapon;

    dboolean		weaponowned[NUMWEAPONS];
    int			ammo[NUMAMMO];
    int			maxammo[NUMAMMO];

    // True if button down last tic.
    int			attackdown;
    int			usedown;

    // Bit flags, for cheats and debug.
    // See cheat_t, above.
    int			cheats;

    // Refired shots are less accurate.
    int			refire;

     // For intermission stats.
    int			killcount;
    int			itemcount;
    int			secretcount;

    // Hint messages.
    char*		message;

    // For screen flashing (red or bright).
    int			damagecount;
    int			bonuscount;

    // Who did damage (NULL for floors/ceilings).
    mobj_t*		attacker;

    // So gun flashes light up areas.
    int			extralight;

    // Current PLAYPAL, ???
    //  can be set to REDCOLORMAP for pain, etc.
    int			fixedcolormap;

    // Player skin colorshift,
    //  0-3 for which color to draw player.
    int			colormap;

    // Overlay view sprites (gun, etc).
    pspdef_t		psprites[NUMPSPRITES];

    // True if secret level has been done.
    dboolean		didsecret;

} player_t;


//
// INTERMISSION
// Structure passed e.g. to WI_Start(wb)
//
typedef struct
{
    dboolean	in;	// whether the player is in game

    // Player stats, kills, collected items etc.
    int		skills;
    int		sitems;
    int		ssecret;
    int		stime;
    int		frags[4];
    int		score;	// current score on entry, modified on return

} wbplayerstruct_t;

typedef struct
{
    int		epsd;	// episode # (0-2)

    // if true, splash the secret level
    dboolean	didsecret;

    // previous and next levels, origin 0
    int		last;
    int		next;

    int		maxkills;
    int		maxitems;
    int		maxsecret;
    int		maxfrags;

    // the par time
    int		partime;

    // index of this player in game
    int		pnum;

    wbplayerstruct_t	plyr[MAXPLAYERS];

} wbstartstruct_t;

#endif