#FUSSY has no effect on the actual code generated
#FUSSY=1
NAME=Doom3D
CC=BCC32
LINK=TLINK32
RC=BRCC32

.autodepend

.path.obj=..\obj
.path.c=.
.path.cpp=.
.path.res=..\obj
.path.rc=.
.path.exe=..
.path.dll=..
.path.def=.
.path.lmp=.

EXE=$(NAME).exe
COMPWAD=CompWad.exe
SETUPEXE=Doom3DSetup.exe
MAKEWAD=MakeWad.exe
SW8DLL=c_sw8.dll
SW16DLL=c_sw16.dll
D3DDLL=c_d3d.dll
DLLDEF=c_dll.def
EXERES=$(NAME).res
SW8RES=c_sw8.res
SW16RES=c_sw16.res
D3DRES=c_d3d.res
COMPRES=compwad.res
SETUPRES=setup.res
MAKEWADRES=makewad.res

#Nasty
COMMONOBJS=m_swap.obj \
	m_fixed.obj

EXEOBJS=doomdef.obj \
	doomstat.obj \
	dstrings.obj \
	tables.obj \
	am_map.obj \
	c_interface.obj \
	co_console.obj \
	d_items.obj \
	d_main.obj \
	d_net.obj \
	f_finale.obj \
	g_game.obj \
	g_actions.obj \
	g_cmds.obj \
	g_settings.obj \
	hu_stuff.obj \
	hu_lib.obj \
	i_input.obj \
	i_main.obj \
	i_music.obj \
	i_net.obj \
	i_reg.obj \
	i_system.obj \
	i_sound.obj \
	i_windoz.obj \
	m_argv.obj \
	m_bbox.obj \
	m_cheat.obj \
	m_keys.obj \
	m_menu.obj \
	m_misc.obj \
	m_random.obj \
	m_shift.obj \
	p_ceilng.obj \
	p_doors.obj \
	p_enemy.obj \
	p_floor.obj \
	p_inter.obj \
	p_lights.obj \
	p_map.obj \
	p_maputl.obj \
	p_mobj.obj \
	p_plats.obj \
	p_pspr.obj \
	p_saveg.obj \
	p_setup.obj \
	p_sight.obj \
	p_spec.obj \
	p_switch.obj \
	p_telept.obj \
	p_tick.obj \
	p_user.obj \
	s_sound.obj \
	sp_split.obj \
	st_lib.obj \
	st_stuff.obj \
	w_compress.obj \
	w_wad.obj \
	wi_stuff.obj \
	z_zone.obj \
	info.obj \
	sounds.obj \
	$(COMMONOBJS)

DLLOBJS=dllmain.obj \
	i_gamma.obj \
	i_overlay.obj \
	tables.obj \
	$(COMMONOBJS)

SWOBJS=c_sw.obj \
	swr_bsp.obj \
	swr_data.obj \
	swr_main.obj \
	swr_plane.obj \
	swr_segs.obj \
	swr_sky.obj \
	swr_things.obj \
	$(DLLOBJS)

SW8OBJS=$(SWOBJS) \
	sw8f_wipe.obj \
	sw8i_video.obj \
	sw8r_draw.obj \
	sw8r_colormaps.obj \
	sw8v_video.obj

SW16OBJS=$(SWOBJS) \
	sw16f_wipe.obj \
	sw16i_video.obj \
	sw16r_draw.obj \
	sw16r_colormaps.obj \
	sw16v_video.obj


D3DOBJS=c_d3d.obj \
	d3df_wipe.obj \
	d3di_video.obj \
	d3dr_bsp.obj \
	d3dr_data.obj \
	d3dr_draw.obj \
	d3dr_main.obj \
	d3dr_md2.obj \
	d3dr_plane.obj \
	d3dr_seg.obj \
	d3dr_things.obj \
	d3dv_video.obj \
	$(DLLOBJS)

COMPOBJS=compwad.obj \
	m_swap.obj \
	w_compress.obj

SETUPOBJS=setup.obj \
	i_reg.obj \
	m_keys.obj

MAKEWADOBJS=makewad.obj

WINOBJ=c0w32.obj
DLLOBJ=c0d32.obj
CONSOBJ=c0x32.obj
WINLIB=import32 cw32mt
DXLIB=$(WINLIB) ddraw dxguid
CONSLIB=import32 cw32mt

LUMPS=m_contrl.lmp

LFLAG=-jC:\PROGRA~1\BORLAND\CBUILDER\LIB;..\obj -x -V4.0
ELFLAG=$(LFLAG) -aa -Tpe
DLFLAG=$(LFLAG) -aa -Tpd
CLFLAG=$(LFLAG) -ap -Tpe

CFLAG=-a4 -w -w-stu -w-par -w-sig -O2 -5 -ff -v-

!if $(FUSSY)
CFLAG=$(CFLAG) -w!
!endif

All: $(EXE) dlls $(COMPWAD) $(SETUPEXE)

dlls: $(SW8DLL) $(SW16DLL) $(D3DDLL)

lumps: $(MAKEWAD) $(LUMPS)

$(EXE): lumps $(EXEOBJS) $(EXERES)
	@echo $<
	@$(LINK) $(ELFLAG) @&&| +
		$(WINOBJ) $(EXEOBJS), +
		$<,, +
		$(DXLIB) dinput,, +
		$(EXERES) +
|

$(SW8DLL): $(SW8OBJS) $(SW8RES)
	@echo $<
	@$(LINK) $(DLFLAG) @&&| +
		$(DLLOBJ) $(SW8OBJS), +
		$<,, +
		$(DXLIB), +
		$(DLLDEF), +
		$(SW8RES) +
|

$(SW16DLL): $(SW16OBJS) $(SW16RES)
	@echo $<
	@$(LINK) $(DLFLAG) @&&| +
		$(DLLOBJ) $(SW16OBJS), +
		$<,, +
		$(DXLIB), +
		$(DLLDEF), +
		$(SW16RES) +
|

$(D3DDLL): $(D3DOBJS) $(D3DRES)
	@echo $<
	@$(LINK) $(DLFLAG) @&&| +
		$(DLLOBJ) $(D3DOBJS), +
		$<,, +
		$(DXLIB), +
		$(DLLDEF), +
		$(D3DRES) +
|

$(COMPWAD): $(COMPOBJS) $(COMPRES)
	@echo $<
	@$(LINK) $(CLFLAG) @&&| +
		$(CONSOBJ) $(COMPOBJS), +
		$<,, +
		$(CONSLIB),, +
		$(COMPRES) +
|

$(MAKEWAD): $(MAKEWADOBJS) $(MAKEWADRES)
	@echo $<
	@$(LINK) $(CLFLAG) @&&| +
		$(CONSOBJ) $(MAKEWADOBJS), +
		$<,, +
		$(CONSLIB),, +
		$(MAKEWADRES) +
|

$(SETUPEXE): $(SETUPOBJS) $(SETUPRES)
	@echo $<
	@$(LINK) $(ELFLAG) @&&| +
		$(WINOBJ) $(SETUPOBJS), +
		$<,, +
		$(WINLIB) ddraw.lib,, +
		$(SETUPRES) +
|

{.}.c.obj:
	@$(CC) $(CFLAG) -o$@ -c $<

{.}.rc.res:
	@$(RC) $(RFLAG) -fo$@ $<

{.}.def.lmp:
	@..\$(MAKEWAD) $<

veryclean: clean
	del ..\$(EXE)

clean:
	del ..\obj\*.obj
	del ..\obj\*.res

touch:
	-@touch *.c *.rc

build: touch all

run: all
	@cd ..
	-@start $(EXE) -doom -warp 1 1 -debug -file c:\pdata\makewad\test.wad
