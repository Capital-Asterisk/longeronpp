#include <gtest/gtest.h>

TEST(Hello, AlwaysPass) {
  EXPECT_STRCASEEQ("hello world, silly bird lol!",
                   "Hello World, silly bird LOL!");
}
