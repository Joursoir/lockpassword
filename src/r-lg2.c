#if defined(LIGBIT)

#include <stdio.h>
#include "r-lg2.h"
#include "xstd.h"
#include "output.h"

git_repository *g_repo = NULL;

static int lg2_commit(git_index *index, char *comment)
{
	int error;
	git_oid commit_oid, tree_oid;
	git_tree *tree = NULL;
	git_object *parent = NULL;
	git_reference *ref = NULL;
	git_signature *author_signature = NULL, *committer_signature = NULL;

	error = git_revparse_ext(&parent, &ref, g_repo, "HEAD");
	if (error == GIT_ENOTFOUND) {
		printf("HEAD not found. Creating first commit\n");
		error = 0;
	} else if (error != 0) {
		const git_error *err = git_error_last();
		if (err) printf("ERROR %d: %s\n", err->klass, err->message);
		else printf("ERROR %d: no detailed info\n", error);
		return error;
	}

	error = git_index_write_tree(&tree_oid, index);
	if (error) {
		print_error("git: Could not write tree\n");
		return error;
	}
	error = git_index_write(index);
	if (error) {
		print_error("git: Could not write index\n");
		return error;
	}

	error = git_tree_lookup(&tree, g_repo, &tree_oid);
	if (error) {
		print_error("git: Error looking up tree\n");
		return error;
	}

	error = git_signature_default_from_env(&author_signature, &committer_signature, g_repo);
	if (error) {
		print_error("git: Error creating signatures\n");
		return error;
	}

	error = git_commit_create_v(
		&commit_oid,
		g_repo,
		"HEAD",
		author_signature,
		committer_signature,
		NULL,
		comment,
		tree,
		parent ? 1 : 0, parent);
	if (error) {
		print_error("git: Error creating commit\n");
	} else {
		char *commit_hash = git_oid_tostr_s(&commit_oid); 
		printf("git: [HEAD %s] %s\n", commit_hash, comment);
		error = 0;
	}

	git_signature_free(author_signature);
	git_signature_free(committer_signature);
	git_tree_free(tree);
	git_object_free(parent);
	git_reference_free(ref);
	return error;
}

int lg2_open_repo(const char *path)
{
	int error = 0;
	error = git_libgit2_init();
	if (error < 0) {
		const git_error *e = git_error_last();
		print_error("git_libgit2_init(): %s\n", e->message);
		return error;
	}

	error = git_repository_open(&g_repo, path);
	if (error < 0) {
		print_error("Warning: could not open repository, trying git init\n");

		error = git_repository_init(&g_repo, path, 0);
		if (error < 0) {
			const git_error *e = git_error_last();
			print_error("git_repository_init(): %s\n", e->message);
			return error;
		}

		git_index *idx = NULL;
		error = git_repository_index(&idx, g_repo);
		if (error < 0) {
			const git_error *e = git_error_last();
			print_error("git_repository_index(): %s\n", e->message);
			git_repository_free(g_repo);
			return error;
		}

		// Add all files in the repository
		char *paths[] = {"*", ".*"};
		git_strarray arr = {paths, 2};

		error = git_index_add_all(idx, &arr, GIT_INDEX_ADD_DEFAULT, NULL, NULL);
		if (error < 0) {
			const git_error *e = git_error_last();
			print_error("git_index_add_all(): %s\n", e->message);
			git_index_free(idx);
			git_repository_free(g_repo);
			return error;
		}

		error = lg2_commit(idx, "init commit");
		git_index_free(idx);
	}
	return error;
}

int lg2_close_repo()
{
	if (g_repo != NULL)
		git_repository_free(g_repo);
	git_libgit2_shutdown();
	return 0;
}

int lg2_simple_action(git_action_t action, int is_overwrite, const char *path, const char *new_path)
{
	int error = 0;
	git_index *idx = NULL;
	char *message = NULL;
	const char *action_str = "";
	const char *overwrite_str = NULL;

	error = git_repository_index(&idx, g_repo);
	if (error < 0) {
		const git_error *e = git_error_last();
		print_error("git_repository_index(): %s\n", e->message);
		return error;
	}

	switch (action) {
	case GIT_ACTION_INSERT:
		action_str = "insert";
		error = git_index_add_bypath(idx, path);
		break;
	case GIT_ACTION_GENERATE:
		action_str = "generate";
		error = git_index_add_bypath(idx, path);
		break;
	case GIT_ACTION_EDIT:
		action_str = "edit";
		error = git_index_add_bypath(idx, path);
		break;
	    
	case GIT_ACTION_DELETE:
		action_str = "delete";
		error = git_index_remove_bypath(idx, path);
		break;
	    
	case GIT_ACTION_MOVE:
		error = git_index_remove_bypath(idx, path);
		if (!error)
			error = git_index_add_bypath(idx, new_path);

		if (is_overwrite)
			overwrite_str = " (overwrite)";
		message = xstrcat(path, " -> ", new_path, overwrite_str, NULL);
		break;
	}

	if (error) {
		const git_error *e = git_error_last();
		print_error("Index operation failed: %s\n", e->message);
		git_index_free(idx);
		return error;
	}

	// Generate commit message if not already set (for move)
	if (!message) {
		if (is_overwrite)
			overwrite_str = " (overwrite)";
		message = xstrcat(path, ": ", action_str, overwrite_str, NULL);
	}

	if (!message) {
		git_index_free(idx);
		return 1;
	}

	error = lg2_commit(idx, message);

	free(message);
	git_index_free(idx);
	return error;
}

#endif /* LIGBIT */
