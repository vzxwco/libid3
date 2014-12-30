/*  Copyright (c) 2006-2014, Philip Busch <vzxwco@gmail.com>
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *   - Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *   - Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "id3.h"

int id3_read(const char *path, struct id3 *id3)
{
	char tmp[TAG_LENGTH];

	assert(path != NULL);
	assert(id3 != NULL);

	if(id3_read_tag(path, tmp) < 0)
	        return(-1);

	if(id3_check_tag(tmp) < 0)
	        return(-1);

	if(id3_extract_info(tmp, id3) < 0)
	        return(-1);

	id3_normalize_info(id3);

	return(0);
}

int id3_write(const char *path, struct id3 *id3)
{
	struct id3 tmp;
	FILE *fp;

	assert(id3 != NULL);
	assert(path != NULL);

	if((fp = fopen(path, "r+")) == NULL)
	        return(-1);

	if(id3_read(path, &tmp) == 0) {
		if(fseek(fp, -1 * TAG_LENGTH, SEEK_END) == -1)
		        return(-1);
	} else {
		if(fseek(fp, 0, SEEK_END) == -1)
		        return(-1);
	}

        id3_write_tag(fp, id3);

	fclose(fp);

	return(0);
}

void id3_print(struct id3 *id3)
{
	char genre[1024];

	assert(id3 != NULL);

	id3_get_genre_as_string(id3, genre);

	printf("Title..: %s\n", id3->title);
	printf("Artist.: %s\n", id3->artist);
	printf("Album..: %s\n", id3->album);
	printf("Year...: %s\n", id3->year);
	printf("Comment: %s\n", id3->comment);
	printf("Genre..: %s (%d)\n", genre, id3->genre);
	printf("Track..: %d\n", id3->track);
}

int id3_read_tag(const char *path, char *tag)
{
	FILE *fp;
	int i;
	
	assert(path != NULL);
	assert(tag != NULL);

	if((fp = fopen(path, "r")) == NULL)
	        return(-1);

	if(fseek(fp, -1 * TAG_LENGTH, SEEK_END) == -1)
	        return(-1);

	for(i = 0; i < TAG_LENGTH; i++) {
		tag[i] = fgetc(fp);
	}

	fclose(fp);

	return(0);
}

int id3_write_tag(FILE *fp, struct id3 *id3)
{
	assert(fp != NULL);
	assert(id3 != NULL);

	fprintf(fp, "TAG");
	write_with_padding(fp, id3->title, SIZE_INFO);
	write_with_padding(fp, id3->artist, SIZE_INFO);
	write_with_padding(fp, id3->album, SIZE_INFO);
	write_with_padding(fp, id3->year, SIZE_YEAR);
	write_with_padding(fp, id3->comment, SIZE_INFO);

	if(id3->track != 0) {
		fseek(fp, -1, SEEK_CUR);
		fprintf(fp, "%c", id3->track);
	}

	fprintf(fp, "%c",   id3->genre);

	return(0);
}

int id3_check_tag(const char *tag)
{
	assert(tag != NULL);

	if((tag[0] != 'T') || (tag[1] != 'A') || (tag[2] != 'G'))
	        return(-1);

	return(0);
}


int id3_extract_info(const char *tag, struct id3 *id3)
{
	assert(tag != NULL);
	assert(id3 != NULL);

	memcpy(id3->title,   tag + OFFSET_TITLE,   SIZE_INFO);
	memcpy(id3->artist,  tag + OFFSET_ARTIST,  SIZE_INFO);
	memcpy(id3->album,   tag + OFFSET_ALBUM,   SIZE_INFO);
	memcpy(id3->year,    tag + OFFSET_YEAR,    SIZE_YEAR);
	memcpy(id3->comment, tag + OFFSET_COMMENT, SIZE_INFO);

	id3->track = (tag[OFFSET_TRACK] == '\0')?0:tag[OFFSET_TRACK];
	id3->genre = tag[OFFSET_GENRE];

	id3->title[30]   = '\0';
	id3->artist[30]  = '\0';
	id3->album[30]   = '\0';
	id3->year[4]     = '\0';
	id3->comment[30] = '\0';

	return(0);
}

static void remove_trailing_whitespaces(char *str)
{
	int i, size;
	
	assert(str != NULL);

	if(str == NULL)
	        return;
	
	size = strlen(str);
	
	for(i = size - 1; i >= 0; i--) {
		if(str[i] == ' ') {
			str[i] = '\0';
		} else break;
	}
}

void id3_normalize_info(struct id3 *id3)
{
	assert(id3 != NULL);

	remove_trailing_whitespaces(id3->title);
	remove_trailing_whitespaces(id3->artist);
	remove_trailing_whitespaces(id3->album);
	remove_trailing_whitespaces(id3->comment);
}

void id3_get_genre_as_string(struct id3 *id3, char *genre)
{
	assert(id3 != NULL);
	assert(genre != NULL);

	if(id3->genre > GENRE_MAX) {
		strcpy(genre, "Unknown");
	} else {
		strcpy(genre, genres[id3->genre]);
	}
}

static void write_with_padding(FILE *fp, const char *str, size_t len)
{
	unsigned int i;

	assert(fp != NULL);
	assert(str != NULL);

	fprintf(fp, "%s", str);

	for(i = 0; i < len - strlen(str); i++) {
		fprintf(fp, "%c", '\0');
	}
}

int main()
{
	struct id3 id3;
	struct id3 tmp;
	
	strcpy(tmp.title, "Englishman in New York");
	strcpy(tmp.artist, "Sting");
	strcpy(tmp.album, "Album");
	strcpy(tmp.year, "Year");
	strcpy(tmp.comment, "Comment");
	tmp.genre = 24;
	tmp.track = 4;

	id3_write("sting.mp3", &tmp);
	id3_read("sting.mp3", &id3);
	id3_print(&id3);

	return(0);
}
