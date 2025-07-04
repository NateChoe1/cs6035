#define NOB_IMPLEMENTATION

#include "nob.h"

#define SRC_DIR "src/"
#define OBJ_DIR "work/"
#define EXE_DIR "build/"
#define EXE_NAME "cc"

#define EXE_PATH EXE_DIR EXE_NAME

#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Werror", \
	"-Wint-conversion", "-ggdb", "-ansi"

int compile_object(const char *src, Nob_File_Paths *headers, Nob_File_Paths *objs);

bool endswith(const char *base, const char *extension);
bool startswith(const char *base, const char *start);
bool contains(const char *str, const char c);

/* equivalent to strdup, just more portable */
char *clone(char *str);

char *append(const char *s1, const char *s2);

bool should_include(const char *filename, const char *target, const char *ext);
void compile_skeleton(char *in, char *out);
void read_line(FILE *in, char *line, size_t size);
void put_escaped_char(int c, FILE *out);

int main(int argc, char **argv) {
	size_t i;
	char *target;

	NOB_GO_REBUILD_URSELF(argc, argv);

	target = (argc >= 2) ? argv[1] : "prod";

	if (!nob_mkdir_if_not_exists(OBJ_DIR)) {
		return 1;
	}

	Nob_File_Paths srcdir = {0};
	Nob_File_Paths headers = {0};
	Nob_File_Paths cfiles = {0};
	Nob_File_Paths objs = {0};

	nob_read_entire_dir(SRC_DIR, &srcdir);

	for (i = 0; i < srcdir.count; ++i) {
		if (should_include(srcdir.items[i], target, ".h")) {
			nob_da_append(&headers, append(SRC_DIR, srcdir.items[i]));
		} else if (should_include(srcdir.items[i], target, ".c")) {
			nob_da_append(&cfiles, srcdir.items[i]);
		} else if (should_include(srcdir.items[i], target, ".skl")) {
			char *p = append(SRC_DIR, srcdir.items[i]);
			nob_da_append(&headers, p);
			compile_skeleton(p, append(p, ".comp"));
		}
	}

	/* the final item in headers is the source file */
	nob_da_append(&headers, "");

	for (i = 0; i < cfiles.count; ++i) {
		if (compile_object(cfiles.items[i], &headers, &objs)) {
			return 1;
		}
	}

	if (!nob_mkdir_if_not_exists(EXE_DIR)) {
		return 1;
	}

	bool needs_rebuild = false;
	needs_rebuild = nob_needs_rebuild(EXE_PATH, objs.items, objs.count);

	Nob_String_Builder cur_target = {0};
	char *tpath = OBJ_DIR "cur_target";
	if (nob_file_exists(tpath) &&
			!nob_read_entire_file(tpath, &cur_target)) {
		return 1;
	}
	nob_sb_append_null(&cur_target);
	if (strcmp(cur_target.items, target)) {
		needs_rebuild = 1;
		if (!nob_write_entire_file(tpath,
					target, strlen(target))) {
			return 1;
		}
	}

	if (!needs_rebuild) {
		goto skip_link;
	}
	Nob_Cmd link = {0};
	nob_cmd_append(&link, "cc", "-o", EXE_PATH);
	for (i = 0; i < objs.count; ++i) {
		nob_da_append(&link, objs.items[i]);
	}
	if (!nob_cmd_run_sync(link)) {
		return 1;
	}
	nob_cmd_free(link);
skip_link:

	for (i = 0; i < headers.count-1; ++i) {
		free((void *) headers.items[i]);
	}
	for (i = 0; i < objs.count; ++i) {
		free((void *) objs.items[i]);
	}

	nob_da_free(srcdir);
	nob_da_free(headers);
	nob_da_free(cfiles);
	nob_da_free(objs);
	return 0;
}

bool endswith(const char *base, const char *extension) {
	size_t elen = strlen(extension);
	return memcmp(base + strlen(base) - elen, extension, elen) == 0;
}

char *clone(char *str) {
	char *ret;
	size_t l;
	l = strlen(str)+1;
	ret = malloc(l);
	assert(ret != NULL);
	memcpy(ret, str, l);
	return ret;
}

int compile_object(const char *src, Nob_File_Paths *headers, Nob_File_Paths *objs) {
	int result = 0;
	Nob_String_Builder obj = {0};
	nob_sb_append_cstr(&obj, OBJ_DIR);
	nob_sb_append_buf(&obj, src, strlen(src) - 2);
	nob_sb_append_cstr(&obj, ".o");
	nob_sb_append_null(&obj);

	char *srcpath = append(SRC_DIR, src);
	headers->items[headers->count-1] = srcpath;

	char *obj_path = obj.items;

	nob_da_append(objs, obj_path);

	if (!nob_needs_rebuild(obj_path, headers->items, headers->count)) {
		result = 0;
		goto end1;
	}

	Nob_Cmd cmd = {0};
	nob_cmd_append(&cmd, "cc", CFLAGS, "-Idist", "-c", "-o", obj_path, srcpath);
	if (!nob_cmd_run_sync(cmd)) {
		result = 1;
		goto end2;
	}

	result = 0;
end2:
	nob_cmd_free(cmd);
end1:
	free(srcpath);
	return result;
}

bool startswith(const char *base, const char *start) {
	size_t prefix = strspn(base, start);
	return start[prefix] == '\0';
}

char *append(const char *s1, const char *s2) {
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	char *ret = malloc(l1 + l2 + 1);
	assert(ret != NULL);
	memcpy(ret, s1, l1);
	memcpy(ret+l1, s2, l2+1);
	return ret;
}

bool should_include(const char *filename, const char *target, const char *ext) {
	if (!endswith(filename, ext)) {
		return false;
	}

	char *suffix = strchr(filename, '_');

	if (suffix == NULL) {
		return true;
	}

	suffix += 1;

	int i;
	for (i = 0; target[i]; ++i) {
		if (suffix[i] != target[i]) {
			return false;
		}
	}
	return strcmp(suffix + i, ext) == 0;
}

void compile_skeleton(char *in_path, char *out_path) {
	if (!nob_needs_rebuild(out_path, (const char **) &in_path, 1)) {
		return;
	}

	FILE *in = fopen(in_path, "r");
	FILE *out = fopen(out_path, "w");
	if (in == NULL || out == NULL) {
		exit(EXIT_FAILURE);
	}

	char head[100];
	char tail[100];
	read_line(in, head, sizeof(head));
	read_line(in, tail, sizeof(head));

	for (;;) {
		int c = fgetc(in);
		if (c == EOF) {
			fflush(out);
			return;
		}
		if (c == '>') {
			for (;;) {
				c = fgetc(in);
				fputc(c, out);
				if (c == '\n') {
					break;
				}
			}
			continue;
		}

		ungetc(c, in);
		fputs(head, out);
		for (;;) {
			int c = fgetc(in);
			if (c == '\n' || c == EOF) {
				break;
			}
			put_escaped_char(c, out);
		}
		fputs(tail, out);
		fputc('\n', out);
	}
}

void read_line(FILE *in, char *line, size_t size) {
	int i;
	for (i = 0; i < size-1; ++i) {
		line[i] = fgetc(in);
		if (line[i] == EOF || line[i] == '\n') {
			break;
		}
	}
	line[i] = '\0';
}

void put_escaped_char(int c, FILE *out) {
	if (c == '\\' || c == '"') {
		fprintf(out, "\\%c", c);
		return;
	}
	fputc(c, out);
}
