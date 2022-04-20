#include <cover/suite.h>

#include <stdlib.h> /* malloc, free */
#include <stdint.h> /* intptr_t */

#include "cyberd/tree.c"

#define TEST_TREE_INTEGER_POOL_MIN INT16_MIN
#define TEST_TREE_INTEGER_POOL_MAX INT16_MAX
#define TEST_TREE_INTEGER_POOL_COUNT (1U + TEST_TREE_INTEGER_POOL_MAX - TEST_TREE_INTEGER_POOL_MIN)

static int
test_tree_compare(const tree_element_t *lhs, const tree_element_t *rhs) {
	return (intptr_t)lhs - (intptr_t)rhs;
}

static intptr_t *test_tree_integer_pool;

static struct tree test_tree = { .compare = test_tree_compare };

void
cover_suite_init(int argc, char **argv) {

	test_tree_integer_pool = malloc(TEST_TREE_INTEGER_POOL_COUNT * sizeof (*test_tree_integer_pool));

	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		test_tree_integer_pool[i] = (intptr_t)i + TEST_TREE_INTEGER_POOL_MIN;
	}
}

void
cover_suite_fini(void) {
	free(test_tree_integer_pool);
}

static void
test_tree_insert(void) {

	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		tree_insert(&test_tree, (tree_element_t *)test_tree_integer_pool[i]);
	}

	/* 16 because the pool count has every possible 16-bits integer, 1 more for the root node */
	cover_assert(test_tree.root->height == 16 + 1, "Tree is not balanced");
}

static void
test_tree_minimum(void) {
	const struct tree_node *min = test_tree.root;

	while (min->left != NULL) {
		min = min->left;
	}

	cover_assert((intptr_t)min->element == TEST_TREE_INTEGER_POOL_MIN, "Minimum element is invalid");
}

static void
test_tree_maximum(void) {
	const struct tree_node *max = test_tree.root;

	while (max->right != NULL) {
		max = max->right;
	}

	cover_assert((intptr_t)max->element == TEST_TREE_INTEGER_POOL_MAX, "Maximum element is invalid");
	cover_assert(max->element == tree_last(&test_tree), "Last element is not maximum");
}

static void
test_tree_remove(void) {
	for (unsigned int i = 0; i < TEST_TREE_INTEGER_POOL_COUNT; i++) {
		tree_remove(&test_tree, (const tree_element_t *)test_tree_integer_pool[i]);
	}
}

const struct cover_case cover_suite[] = {
	COVER_SUITE_TEST(test_tree_insert),
	COVER_SUITE_TEST(test_tree_minimum),
	COVER_SUITE_TEST(test_tree_maximum),
	COVER_SUITE_TEST(test_tree_remove),
	COVER_SUITE_END,
};
