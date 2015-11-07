//
// Copyright(C) 2005-2014 Simon Howard
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// DESCRIPTION:
//    Reading of MIDI files.
//


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "c_io.h"

#include "doom/doomdef.h"

#include "doomfeatures.h"
#include "doomtype.h"
#include "i_swap.h"
#include "midifile.h"
#include "v_trans.h"

#include "z_zone.h"


#define HEADER_CHUNK_ID "MThd"
#define TRACK_CHUNK_ID  "MTrk"
#define MAX_BUFFER_SIZE 0x10000


typedef struct
{
    byte chunk_id[4];
    unsigned int chunk_size;
} PACKEDATTR chunk_header_t;

typedef struct
{
    chunk_header_t chunk_header;
    unsigned short format_type;
    unsigned short num_tracks;
    unsigned short time_division;
} PACKEDATTR midi_header_t;

typedef struct
{
    // Length in bytes:

    unsigned int data_len;

    // Events in this track:

    midi_event_t *events;
    int num_events;
} midi_track_t;

struct midi_track_iter_s
{
    midi_track_t *track;
    unsigned int position;
};

struct midi_file_s
{
    midi_header_t header;

    // All tracks in this file:
    midi_track_t *tracks;
    unsigned int num_tracks;

    // Data buffer used to store data read for SysEx or meta events:
    byte *buffer;
    unsigned int buffer_size;
};


// Check the header of a chunk:

static dboolean CheckChunkHeader(chunk_header_t *chunk,
                                char *expected_id)
{
    dboolean result;
    
    result = (memcmp((char *) chunk->chunk_id, expected_id, 4) == 0);

    if (!result)
    {
        C_Printf(CR_RED, " CheckChunkHeader: Expected '%s' chunk header, "
                        "got '%c%c%c%c'\n",
                        expected_id,
                        chunk->chunk_id[0], chunk->chunk_id[1],
                        chunk->chunk_id[2], chunk->chunk_id[3]);
    }

    return result;
}

// Read a single byte.  Returns false on error.

static dboolean ReadByte(byte *result, FILE *stream)
{
    int c;

    c = fgetc(stream);

    if (c == EOF)
    {
        C_Printf(CR_RED, " ReadByte: Unexpected end of file\n");
        return false;
    }
    else
    {
        *result = (byte) c;

        return true;
    }
}

// Read a variable-length value.

static dboolean ReadVariableLength(unsigned int *result, FILE *stream)
{
    int i;
    byte b = 0;

    *result = 0;

    for (i=0; i<4; ++i)
    {
        if (!ReadByte(&b, stream))
        {
            C_Printf(CR_RED, " ReadVariableLength: Error while reading "
                            "variable-length value\n");
            return false;
        }

        // Insert the bottom seven bits from this byte.

        *result <<= 7;
        *result |= b & 0x7f;

        // If the top bit is not set, this is the end.

        if ((b & 0x80) == 0)
        {
            return true;
        }
    }

    C_Printf(CR_RED, " ReadVariableLength: Variable-length value too "
                    "long: maximum of four bytes\n");
    return false;
}

// Read a byte sequence into the data buffer.

static void *ReadByteSequence(unsigned int num_bytes, FILE *stream)
{
    unsigned int i;
    byte *result;

    // Allocate a buffer. Allocate one extra byte, as malloc(0) is
    // non-portable.

    result = malloc(num_bytes + 1);

    if (result == NULL)
    {
        C_Printf(CR_RED, " ReadByteSequence: Failed to allocate buffer\n");
        return NULL;
    }

    // Read the data:

    for (i=0; i<num_bytes; ++i)
    {
        if (!ReadByte(&result[i], stream))
        {
            C_Printf(CR_RED, " ReadByteSequence: Error while reading byte %u\n",
                            i);
            free(result);
            return NULL;
        }
    }

    return result;
}

// Read a MIDI channel event.
// two_param indicates that the event type takes two parameters
// (three byte) otherwise it is single parameter (two byte)

static dboolean ReadChannelEvent(midi_event_t *event,
                                byte event_type, dboolean two_param,
                                FILE *stream)
{
    byte b = 0;

    // Set basics:

    event->event_type = event_type & 0xf0;
    event->data.channel.channel = event_type & 0x0f;

    // Read parameters:

    if (!ReadByte(&b, stream))
    {
        C_Printf(CR_RED, " ReadChannelEvent: Error while reading channel "
                        "event parameters\n");
        return false;
    }

    event->data.channel.param1 = b;

    // Second parameter:

    if (two_param)
    {
        if (!ReadByte(&b, stream))
        {
            C_Printf(CR_RED, " ReadChannelEvent: Error while reading channel "
                            "event parameters\n");
            return false;
        }

        event->data.channel.param2 = b;
    }

    return true;
}

// Read sysex event:

static dboolean ReadSysExEvent(midi_event_t *event, int event_type,
                              FILE *stream)
{
    event->event_type = event_type;

    if (!ReadVariableLength(&event->data.sysex.length, stream))
    {
        C_Printf(CR_RED, " ReadSysExEvent: Failed to read length of "
                                        "SysEx block\n");
        return false;
    }

    // Read the byte sequence:

    event->data.sysex.data = ReadByteSequence(event->data.sysex.length, stream);

    if (event->data.sysex.data == NULL)
    {
        C_Printf(CR_RED, " ReadSysExEvent: Failed while reading SysEx event\n");
        return false;
    }

    return true;
}

// Read meta event:

static dboolean ReadMetaEvent(midi_event_t *event, FILE *stream)
{
    byte b = 0;

    event->event_type = MIDI_EVENT_META;

    // Read meta event type:

    if (!ReadByte(&b, stream))
    {
        C_Printf(CR_RED, " ReadMetaEvent: Failed to read meta event type\n");
        return false;
    }

    event->data.meta.type = b;

    // Read length of meta event data:

    if (!ReadVariableLength(&event->data.meta.length, stream))
    {
        C_Printf(CR_RED, " ReadSysExEvent: Failed to read length of "
                                        "SysEx block\n");
        return false;
    }

    // Read the byte sequence:

    event->data.meta.data = ReadByteSequence(event->data.meta.length, stream);

    if (event->data.meta.data == NULL)
    {
        C_Printf(CR_RED, " ReadSysExEvent: Failed while reading SysEx event\n");
        return false;
    }

    return true;
}

static dboolean ReadEvent(midi_event_t *event, unsigned int *last_event_type,
                         FILE *stream)
{
    byte event_type = 0;

    if (!ReadVariableLength(&event->delta_time, stream))
    {
        C_Printf(CR_RED, " ReadEvent: Failed to read event timestamp\n");
        return false;
    }

    if (!ReadByte(&event_type, stream))
    {
        C_Printf(CR_RED, " ReadEvent: Failed to read event type\n");
        return false;
    }

    // All event types have their top bit set.  Therefore, if 
    // the top bit is not set, it is because we are using the "same
    // as previous event type" shortcut to save a byte.  Skip back
    // a byte so that we read this byte again.

    if ((event_type & 0x80) == 0)
    {
        event_type = *last_event_type;

        if (fseek(stream, -1, SEEK_CUR) < 0)
        {
            C_Printf(CR_RED, " ReadEvent: Unable to seek in stream\n");
            return false;
        }
    }
    else
    {
        *last_event_type = event_type;
    }

    // Check event type:

    switch (event_type & 0xf0)
    {
        // Two parameter channel events:

        case MIDI_EVENT_NOTE_OFF:
        case MIDI_EVENT_NOTE_ON:
        case MIDI_EVENT_AFTERTOUCH:
        case MIDI_EVENT_CONTROLLER:
        case MIDI_EVENT_PITCH_BEND:
            return ReadChannelEvent(event, event_type, true, stream);

        // Single parameter channel events:

        case MIDI_EVENT_PROGRAM_CHANGE:
        case MIDI_EVENT_CHAN_AFTERTOUCH:
            return ReadChannelEvent(event, event_type, false, stream);

        default:
            break;
    }

    // Specific value?

    switch (event_type)
    {
        case MIDI_EVENT_SYSEX:
        case MIDI_EVENT_SYSEX_SPLIT:
            return ReadSysExEvent(event, event_type, stream);

        case MIDI_EVENT_META:
            return ReadMetaEvent(event, stream);

        default:
            break;
    }

    C_Printf(CR_RED, " ReadEvent: Unknown MIDI event type: 0x%x\n", event_type);
    return false;
}

// Free an event:

static void FreeEvent(midi_event_t *event)
{
    // Some event types have dynamically allocated buffers assigned
    // to them that must be freed.

    switch (event->event_type)
    {
        case MIDI_EVENT_SYSEX:
        case MIDI_EVENT_SYSEX_SPLIT:
            free(event->data.sysex.data);
            break;

        case MIDI_EVENT_META:
            free(event->data.meta.data);
            break;

        default:
            // Nothing to do.
            break;
    }
}

// Read and check the track chunk header

static dboolean ReadTrackHeader(midi_track_t *track, FILE *stream)
{
    size_t records_read;
    chunk_header_t chunk_header;

    records_read = fread(&chunk_header, sizeof(chunk_header_t), 1, stream);

    if (records_read < 1)
    {
        C_Printf(CR_RED, " ReadTrackHeader: records_read is < 1\n");
        return false;
    }

    if (!CheckChunkHeader(&chunk_header, TRACK_CHUNK_ID))
    {
        C_Printf(CR_RED, " ReadTrackHeader: Error (CheckChunkHeader)\n");
        return false;
    }

    track->data_len = SDL_SwapBE32(chunk_header.chunk_size);

    return true;
}

static dboolean ReadTrack(midi_track_t *track, FILE *stream)
{
    midi_event_t *new_events;
    unsigned int last_event_type;

    track->num_events = 0;
    track->events = NULL;

    // Read the header:

    if (!ReadTrackHeader(track, stream))
    {
        C_Printf(CR_RED, " ReadTrack: Error (Read the header)\n");
        return false;
    }

    // Then the events:

    last_event_type = 0;

    for (;;)
    {
        midi_event_t *event;

        // Resize the track slightly larger to hold another event:
#ifdef BOOM_ZONE_HANDLING
        new_events = Z_Realloc(track->events, 
                             sizeof(midi_event_t) * (track->num_events + 1), PU_CACHE, NULL);
#else
        new_events = Z_Realloc(track->events, 
                             sizeof(midi_event_t) * (track->num_events + 1));
#endif
/*
        new_events = Z_Realloc(track->events, 
                             sizeof(midi_event_t) * (track->num_events + 1), PU_STATIC, NULL);
*/
        if (new_events == NULL)
        {
            C_Printf(CR_RED, " ReadTrack: Error (new_events is NULL)\n");
            C_Printf(CR_RED, " ReadTrack: Error (track->num_events is %d)\n", track->num_events);
            return false;
        }

        track->events = new_events;

        // Read the next event:

        event = &track->events[track->num_events];
        if (!ReadEvent(event, &last_event_type, stream))
        {
            C_Printf(CR_RED, " ReadTrack: Error (Read the next event)\n");
            return false;
        }

        ++track->num_events;

        // End of track?

        if (event->event_type == MIDI_EVENT_META
         && event->data.meta.type == MIDI_META_END_OF_TRACK)
        {
            break;
        }
    }

    return true;
}

// Free a track:

static void FreeTrack(midi_track_t *track)
{
    unsigned int i;

    for (i=0; i<track->num_events; ++i)
    {
        FreeEvent(&track->events[i]);
    }

    free(track->events);
}

static dboolean ReadAllTracks(midi_file_t *file, FILE *stream)
{
    unsigned int i;

    // Allocate list of tracks and read each track:

    file->tracks = malloc(sizeof(midi_track_t) * file->num_tracks);

    if (file->tracks == NULL)
    {
        C_Printf(CR_RED, " ReadAllTracks: file->tracks is NULL\n");
        return false;
    }

    memset(file->tracks, 0, sizeof(midi_track_t) * file->num_tracks);

    // Read each track:

    for (i=0; i<file->num_tracks; ++i)
    {
        if (!ReadTrack(&file->tracks[i], stream))
        {
            C_Printf(CR_RED, " ReadAllTracks: Error (Read each track)\n");
            return false;
        }
    }

    return true;
}

// Read and check the header chunk.

static dboolean ReadFileHeader(midi_file_t *file, FILE *stream)
{
    size_t records_read;
    unsigned int format_type;

    records_read = fread(&file->header, sizeof(midi_header_t), 1, stream);

    if (records_read < 1)
    {
        C_Printf(CR_RED, " ReadFileHeader: records_read is < 1\n");
        return false;
    }

    if (!CheckChunkHeader(&file->header.chunk_header, HEADER_CHUNK_ID)
     || SDL_SwapBE32(file->header.chunk_header.chunk_size) != 6)
    {
        C_Printf(CR_RED, " ReadFileHeader: Invalid MIDI chunk header! "
                        "chunk_size=%i\n",
                        SDL_SwapBE32(file->header.chunk_header.chunk_size));
        return false;
    }

    format_type = SDL_SwapBE16(file->header.format_type);
    file->num_tracks = SDL_SwapBE16(file->header.num_tracks);

    if ((format_type != 0 && format_type != 1)
     || file->num_tracks < 1)
    {
        C_Printf(CR_RED, " ReadFileHeader: Only type 0/1 "
                                         "MIDI files supported!\n");
        return false;
    }

    return true;
}

void MIDI_FreeFile(midi_file_t *file)
{
    if (file->tracks != NULL)
    {
        unsigned int    i;

        for (i=0; i<file->num_tracks; ++i)
        {
            FreeTrack(&file->tracks[i]);
        }

        free(file->tracks);
    }

    free(file);
}

midi_file_t *MIDI_LoadFile(char *filename)
{
    midi_file_t *file;
    FILE *stream;

    file = malloc(sizeof(midi_file_t));

    if (file == NULL)
    {
        C_Printf(CR_RED, " MIDI_LoadFile: file is NULL for file %c\n", filename);
        return NULL;
    }

    file->tracks = NULL;
    file->num_tracks = 0;
    file->buffer = NULL;
    file->buffer_size = 0;

    // Open file

    stream = fopen(filename, "rb");

    if (stream == NULL)
    {
        C_Printf(CR_RED, " MIDI_LoadFile: Failed to open '%s'\n", filename);
        MIDI_FreeFile(file);
        return NULL;
    }

    // Read MIDI file header

    if (!ReadFileHeader(file, stream))
    {
        C_Printf(CR_RED, " MIDI_LoadFile: Error (Read MIDI file header)\n");
        fclose(stream);
        MIDI_FreeFile(file);
        return NULL;
    }

    // Read all tracks:

    if (!ReadAllTracks(file, stream))
    {
        C_Printf(CR_RED, " MIDI_LoadFile: Error (Read all tracks)\n");
        fclose(stream);
        MIDI_FreeFile(file);
        return NULL;
    }

    fclose(stream);

    return file;
}

// Get the number of tracks in a MIDI file.

unsigned int MIDI_NumTracks(midi_file_t *file)
{
    return file->num_tracks;
}

// Start iterating over the events in a track.

midi_track_iter_t *MIDI_IterateTrack(midi_file_t *file, unsigned int track)
{
    midi_track_iter_t *iter;

    assert(track < file->num_tracks);

    iter = malloc(sizeof(*iter));
    iter->track = &file->tracks[track];
    iter->position = 0;

    return iter;
}

void MIDI_FreeIterator(midi_track_iter_t *iter)
{
    free(iter);
}

// Get the time until the next MIDI event in a track.

unsigned int MIDI_GetDeltaTime(midi_track_iter_t *iter)
{
    if (iter->position < iter->track->num_events)
    {
        midi_event_t *next_event;

        next_event = &iter->track->events[iter->position];

        return next_event->delta_time;
    }
    else
    {
        return 0;
    }
}

// Get a pointer to the next MIDI event.

int MIDI_GetNextEvent(midi_track_iter_t *iter, midi_event_t **event)
{
    if (iter->position < iter->track->num_events)
    {
        *event = &iter->track->events[iter->position];
        ++iter->position;

        return 1;
    }
    else
    {
        return 0;
    }
}

unsigned int MIDI_GetFileTimeDivision(midi_file_t *file)
{
    short result = SDL_SwapBE16(file->header.time_division);

    // Negative time division indicates SMPTE time and must be handled
    // differently.
    if (result < 0)
    {
        return (signed int)(-(result/256))
             * (signed int)(result & 0xFF);
    }
    else
    {
        return result;
    }
}

void MIDI_RestartIterator(midi_track_iter_t *iter)
{
    iter->position = 0;
}

