#include <tiny_csv/task_queue.hpp>

#include <gtest/gtest.h>

namespace /* anonymous */
{

TEST(TaskQueue, Basic) {
    std::atomic<int> called(0);
    {
        tiny_csv::TaskQueue<int> queue(4);
        for (int i = 0; i < 64; ++i) {
            queue.Enqueue([&called](int param) {
                called.fetch_add(param);
            }, 1);
        }
    }

    EXPECT_EQ(called.load(), 64);
}


}