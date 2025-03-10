#define NOB_IMPLEMENTATION

#include "nob.h"

#define SRC_DIR "src/"
#define OBJ_DIR "work/"
#define EXE_DIR "build/"
#define EXE_NAME "cc"

#define EXE_PATH EXE_DIR EXE_NAME

#define CFLAGS "-Wall", "-Wextra", "-Wpedantic", "-Werror", "-ggdb", "-ansi"

int compile_object(const char *src, Nob_File_Paths *headers, Nob_File_Paths *objs);

bool endswith(const char *base, const char *extension);

/* equivalent to strdup, just more portable */
char *clone(char *str);

char *append(const char *s1, const char *s2);

int main(int argc, char **argv) {
	size_t i;
	NOB_GO_REBUILD_URSELF(argc, argv);

	if (!nob_mkdir_if_not_exists(OBJ_DIR)) {
		return 1;
	}

	Nob_File_Paths srcdir = {0};
	Nob_File_Paths headers = {0};
	Nob_File_Paths cfiles = {0};
	Nob_File_Paths objs = {0};

	nob_read_entire_dir(SRC_DIR, &srcdir);

	for (i = 0; i < srcdir.count; ++i) {
		if (endswith(srcdir.items[i], ".h")) {
			nob_da_append(&headers, append(SRC_DIR, srcdir.items[i]));
		} else if (endswith(srcdir.items[i], ".c")) {
			nob_da_append(&cfiles, srcdir.items[i]);
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
	if (!nob_needs_rebuild(EXE_PATH, objs.items, objs.count)) {
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
	nob_cmd_append(&cmd, "cc", CFLAGS, "-c", "-o", obj_path, srcpath);
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

char *append(const char *s1, const char *s2) {
	size_t l1 = strlen(s1);
	size_t l2 = strlen(s2);
	char *ret = malloc(l1 + l2 + 1);
	assert(ret != NULL);
	memcpy(ret, s1, l1);
	memcpy(ret+l1, s2, l2+1);
	return ret;
}
