#include <stdlib.h> /* malloc, free */
#include <stdint.h> /* intptr_t */
#include <err.h> /* errx */

#include "cyberd/tree.c"

#define TEST_TREE_INTEGER_POOL_MIN INT16_MIN
#define TEST_TREE_INTEGER_POOL_MAX INT16_MAX
#define TEST_TREE_INTEGER_POOL_COUNT (1U + TEST_TREE_INTEGER_POOL_MAX - TEST_TREE_INTEGER_POOL_MIN)

static int
test_tree_compare(const tree_element_t *lhs, const tree_element_t *rhs) {
	return (intptr_t)lhs - (intptr_t)rhs;
}

int
main(int argc, char *argv[]) {
	struct tree test_tree = { .compare = test_tree_compare };
	intptr_t *test_tree_integer_pool;

	/******************
	 * Initialization *
	 ******************/
	test_tree_integer_pool = malloc(TEST_TREE_INTEGER_POOL_COUNT * sizeof (*test_tree_integer_pool));
	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		test_tree_integer_pool[i] = (intptr_t)i + TEST_TREE_INTEGER_POOL_MIN;
	}

	/**************
	 * Insertions *
	 **************/
	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		tree_insert(&test_tree, (tree_element_t *)test_tree_integer_pool[i]);
	}
	/* 16 because the pool count has every possible 16-bits integer, 1 more for the root node */
	if (test_tree.root->height != 16 + 1) {
		errx(EXIT_FAILURE, "Tree is not balanced");
	}

	/***********
	 * Minimum *
	 ***********/
	const struct tree_node *min = test_tree.root;
	while (min->left != NULL) {
		min = min->left;
	}
	if ((intptr_t)min->element != TEST_TREE_INTEGER_POOL_MIN) {
		errx(EXIT_FAILURE, "Minimum element is invalid");
	}

	/***********
	 * Maximum *
	 ***********/
	const struct tree_node *max = test_tree.root;
	while (max->right != NULL) {
		max = max->right;
	}
	if ((intptr_t)max->element != TEST_TREE_INTEGER_POOL_MAX) {
		errx(EXIT_FAILURE, "Maximum element is invalid");
	}
	if (max->element != tree_last(&test_tree)) {
		errx(EXIT_FAILURE, "Last element is not maximum");
	}

	/***********
	 * Removal *
	 ***********/
	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		tree_remove(&test_tree, (const tree_element_t *)test_tree_integer_pool[i]);
	}

	/****************
	 * Finalization *
	 ****************/
	free(test_tree_integer_pool);

	return EXIT_SUCCESS;
}
