#include "MapReader.h"

#include "map.h"
#include "FileIO.h"
#include "TilesetManager.h"

#include <iostream>

extern CTilesetManager* g_tilesetmanager;
extern const char* g_szBackgroundConversion[26];
extern short g_iMusicCategoryConversion[26];
extern short g_iTileTypeConversion[NUMTILETYPES];
extern short g_iDefaultPowerupPresets[NUM_POWERUP_PRESETS][NUM_POWERUPS];

using namespace std;

MapReader1700::MapReader1700()
    : MapReader1600()
{
    patch_version = 0;
}

MapReader1701::MapReader1701()
    : MapReader1700()
{
    patch_version = 1;
}

MapReader1702::MapReader1702()
    : MapReader1701()
{
    patch_version = 2;
}

void MapReader1702::read_autofilters(CMap& map, FILE* mapfile)
{
    int iAutoFilterValues[9];
    ReadIntChunk(iAutoFilterValues, 9, mapfile);

    for (short iFilter = 0; iFilter < 8; iFilter++)
        map.fAutoFilter[iFilter] = iAutoFilterValues[iFilter] > 0;

    for (short iFilter = 8; iFilter < NUM_AUTO_FILTERS; iFilter++)
        map.fAutoFilter[iFilter] = false;

    //Read density and don't do anything with it at the moment
    //int iDensity = iAutoFilterValues[8];
}

void MapReader1700::read_tiles(CMap& map, FILE* mapfile)
{
    short iClassicTilesetID = g_tilesetmanager->GetIndexFromName("Classic");

    unsigned short i, j, k;
    for (j = 0; j < MAPHEIGHT; j++) {
        for (i = 0; i < MAPWIDTH; i++) {
            for (k = 0; k < MAPLAYERS; k++) {
                short iTileID = (short)ReadInt(mapfile);

                TilesetTile * tile = &map.mapdata[i][j][k];

                if (iTileID == TILESETSIZE) {
                    tile->iID = TILESETNONE;
                    tile->iCol = 0;
                    tile->iRow = 0;
                } else {
                    tile->iID = iClassicTilesetID;
                    tile->iCol = iTileID % TILESETWIDTH;
                    tile->iRow = iTileID / TILESETWIDTH;
                }
            }

            map.objectdata[i][j].iType = (short)ReadInt(mapfile);
            if (map.objectdata[i][j].iType == 15)
                map.objectdata[i][j].iType = -1;

            map.objectdata[i][j].fHidden = false;

            if (map.objectdata[i][j].iType == 1) {
                for (short iSetting = 0; iSetting < NUM_BLOCK_SETTINGS; iSetting++)
                    map.objectdata[i][j].iSettings[iSetting] = g_iDefaultPowerupPresets[0][iSetting];
            }
        }
    }
}

void MapReader1701::read_background(CMap& map, FILE* mapfile)
{
    //Read in background to use
    ReadString(map.szBackgroundFile, 128, mapfile);

    for (short iBackground = 0; iBackground < 26; iBackground++) {
        const char * szFindUnderscore = strstr(g_szBackgroundConversion[iBackground], "_");

        if (szFindUnderscore)
            szFindUnderscore++;

        if (!strcmp(szFindUnderscore, map.szBackgroundFile)) {
            strcpy(map.szBackgroundFile, g_szBackgroundConversion[iBackground]);
        }
    }
}

void MapReader1702::read_background(CMap& map, FILE* mapfile)
{
    //Read in background to use
    ReadString(map.szBackgroundFile, 128, mapfile);
    //printf("Background: %s", szBackgroundFile);
}

void MapReader1700::set_preview_switches(CMap& map, FILE* mapfile)
{
    // if it is a preview, for older maps, set the on/off blocks to on by default

    // FIXME
    /*if (iReadType != read_type_preview)
        return;*/

    //Read on/off switches
    for (unsigned short iSwitch = 0; iSwitch < 4; iSwitch++) {
        map.iSwitches[iSwitch] = 1;
    }

    //Set all the on/off blocks correctly
    for (unsigned short j = 0; j < MAPHEIGHT; j++) {
        for (unsigned short i = 0; i < MAPWIDTH; i++) {
            if (map.objectdata[i][j].iType >= 11 && map.objectdata[i][j].iType <= 14) {
                map.objectdata[i][j].iSettings[0] = 1;
            }
        }
    }
}

void MapReader1700::read_switches(CMap& map, FILE* mapfile)
{
    //Read on/off switches
    for (unsigned short iSwitch = 0; iSwitch < 4; iSwitch++) {
        map.iSwitches[iSwitch] = 1 - (short)ReadInt(mapfile);
    }

    //Set all the on/off blocks correctly
    for (unsigned short j = 0; j < MAPHEIGHT; j++) {
        for (unsigned short i = 0; i < MAPWIDTH; i++) {
            if (map.objectdata[i][j].iType >= 11 && map.objectdata[i][j].iType <= 14) {
                map.objectdata[i][j].iSettings[0] = map.iSwitches[map.objectdata[i][j].iType - 11];
            }
        }
    }
}

void MapReader1701::read_music_category(CMap& map, FILE* mapfile)
{
    map.musicCategoryID = ReadInt(mapfile);
}

void MapReader1700::read_warp_locations(CMap& map, FILE* mapfile)
{
    for (unsigned short j = 0; j < MAPHEIGHT; j++) {
        for (unsigned short i = 0; i < MAPWIDTH; i++) {
            TileType iType = (TileType)ReadInt(mapfile);

            if (iType >= 0 && iType < NUMTILETYPES) {
                map.mapdatatop[i][j].iType = iType;
                map.mapdatatop[i][j].iFlags = g_iTileTypeConversion[iType];
            } else {
                map.mapdatatop[i][j].iType = tile_nonsolid;
                map.mapdatatop[i][j].iFlags = tile_flag_nonsolid;
            }

            map.warpdata[i][j].direction = (WarpEnterDirection)ReadInt(mapfile);
            map.warpdata[i][j].connection = (short)ReadInt(mapfile);
            map.warpdata[i][j].id = (short)ReadInt(mapfile);

            for (short sType = 0; sType < 6; sType += 5)
                map.nospawn[sType][i][j] = ReadInt(mapfile) == 0 ? false : true;

            //Copy player no spawn areas into team no spawn areas
            for (short sType = 1; sType < 5; sType++)
                map.nospawn[sType][i][j] = map.nospawn[0][i][j];

        }
    }
}

bool MapReader1700::read_spawn_areas(CMap& map, FILE* mapfile)
{
    //Read spawn areas
    for (unsigned short i = 0; i < 6; i += 5) {
        map.totalspawnsize[i] = 0;
        map.numspawnareas[i] = (short)ReadInt(mapfile);

        if (map.numspawnareas[i] > MAXSPAWNAREAS) {
            cout << endl << " ERROR: Number of spawn areas (" << map.numspawnareas[i]
                 << ") was greater than max allowed (" << MAXSPAWNAREAS << ')'
                 << endl;
            return false;
        }

        for (int m = 0; m < map.numspawnareas[i]; m++) {
            map.spawnareas[i][m].left = (short)ReadInt(mapfile);
            map.spawnareas[i][m].top = (short)ReadInt(mapfile);
            map.spawnareas[i][m].width = (short)ReadInt(mapfile);
            map.spawnareas[i][m].height = (short)ReadInt(mapfile);
            map.spawnareas[i][m].size = (short)ReadInt(mapfile);

            map.totalspawnsize[i] += map.spawnareas[i][m].size;
        }
    }

    //Copy player spawn areas to team specific spawn areas
    for (short iType = 1; iType < 5; iType++) {
        map.totalspawnsize[iType] = map.totalspawnsize[0];
        map.numspawnareas[iType] = map.numspawnareas[0];

        for (int m = 0; m < map.numspawnareas[0]; m++) {
            map.spawnareas[iType][m].left = map.spawnareas[0][m].left;
            map.spawnareas[iType][m].top = map.spawnareas[0][m].top;
            map.spawnareas[iType][m].width = map.spawnareas[0][m].width;
            map.spawnareas[iType][m].height = map.spawnareas[0][m].height;
            map.spawnareas[iType][m].size = map.spawnareas[0][m].size;
        }
    }

    return true;
}

bool MapReader1700::load(CMap& map, FILE* mapfile, ReadType readtype)
{
    read_autofilters(map, mapfile);

    if (readtype == read_type_summary)
        return true;

    map.clearPlatforms();

    read_tiles(map, mapfile);
    read_background(map, mapfile);

    // TODO: refactor CMap::loadPlatform to not need this
    int mapversion[4] = {1, 7, 0, patch_version};

    if (patch_version >= 1)
        read_switches(map, mapfile);
    else if (readtype != read_type_preview)
        set_preview_switches(map, mapfile);

    if (patch_version >= 2) {
        //short translationid[1] = {g_tilesetmanager->GetIndexFromName("Classic")};
        map.loadPlatforms(mapfile, readtype == read_type_preview, mapversion);
    }

    read_eyecandy(map, mapfile);
    read_music_category(map, mapfile);
    read_warp_locations(map, mapfile);

    if (readtype == read_type_preview)
        return true;

    read_warp_exits(map, mapfile);
    read_spawn_areas(map, mapfile);
    if (!read_draw_areas(map, mapfile))
        return false;

    if (patch_version <= 1) {
        //short translationid[1] = {g_tilesetmanager->GetIndexFromName("Classic")};
        map.loadPlatforms(mapfile, readtype == read_type_preview, mapversion);
    }

    if (patch_version == 0)
        read_switches(map, mapfile);

    return true;
}
