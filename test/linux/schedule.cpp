#include <os.h>

#include <gtest/gtest.h>

class ScheduleSuite : public ::testing::Test {
protected:
    virtual void SetUp();
    virtual void TearDown();

};

void ScheduleSuite::SetUp() {
}

void ScheduleSuite::TearDown() {
    ASSERT_EQ(os_teardown(), OSS_SUCCESS);
}

static void task_handler_idle(void *p);
static void task_handler_test(void *p);

TEST_F(ScheduleSuite, Initialize_OneTask) {
    os_task_t tasks[1];
    uint32_t stacks[1][OS_STACK_MINIMUM_SIZE_WORDS];

    ASSERT_EQ(os_initialize(), OSS_SUCCESS);
    ASSERT_EQ(os_task_initialize(&tasks[0], "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, stacks[0], sizeof(stacks[0])), OSS_SUCCESS);
    ASSERT_EQ(os_start(), OSS_SUCCESS);

    ASSERT_EQ(osg.ntasks, 1);
    ASSERT_EQ(osg.idle, &tasks[0]);
    ASSERT_EQ(osg.runqueue, &tasks[0]);
    ASSERT_EQ(osg.running, &tasks[0]);
    ASSERT_EQ(osg.scheduled, nullptr);

    ASSERT_EQ(tasks[0].status, OS_TASK_STATUS_IDLE);
    ASSERT_EQ(tasks[0].stack_size, sizeof(stacks[0]));
    ASSERT_EQ(tasks[0].stack, &stacks[0][0]);
    ASSERT_EQ(tasks[0].sp, &stacks[0][OS_STACK_MINIMUM_SIZE_WORDS - OS_STACK_BASIC_FRAME_SIZE]);
    ASSERT_STREQ(tasks[0].name, "idle");
    ASSERT_EQ(tasks[0].priority, OS_PRIORITY_IDLE);
    ASSERT_EQ(tasks[0].handler, &task_handler_idle);
    ASSERT_EQ(tasks[0].flags, 0);
    ASSERT_EQ(tasks[0].delay, 0);
    ASSERT_EQ(tasks[0].queue, nullptr);
    ASSERT_EQ(tasks[0].mutex, nullptr);
    ASSERT_EQ(tasks[0].nblocked, nullptr);
    ASSERT_EQ(tasks[0].nrp, nullptr);
    ASSERT_EQ(tasks[0].np, nullptr);

    auto sp = (os_our_sframe_t *)tasks[0].sp;
    ASSERT_EQ(sp->basic.xpsr, 0x01000000);
    ASSERT_EQ(sp->basic.pc, (uint32_t)(uintptr_t)task_handler_idle);
    ASSERT_NE(sp->basic.lr, 0x0);
    ASSERT_EQ(sp->basic.r0, 0x0);
    ASSERT_EQ(stacks[0][0], OSH_STACK_MAGIC_WORD);
}

TEST_F(ScheduleSuite, Initialize_TwoTasks) {
    os_task_t tasks[2];
    uint32_t stacks[2][OS_STACK_MINIMUM_SIZE_WORDS];

    ASSERT_EQ(os_initialize(), OSS_SUCCESS);
    ASSERT_EQ(os_task_initialize(&tasks[0], "idle", OS_TASK_START_RUNNING, &task_handler_idle, NULL, stacks[0], sizeof(stacks[0])), OSS_SUCCESS);
    ASSERT_EQ(os_task_initialize(&tasks[1], "task0", OS_TASK_START_RUNNING, &task_handler_test, NULL, stacks[1], sizeof(stacks[1])), OSS_SUCCESS);
    ASSERT_EQ(os_start(), OSS_SUCCESS);

    ASSERT_EQ(osg.ntasks, 2);
    ASSERT_EQ(osg.idle, &tasks[0]);
    ASSERT_EQ(osg.runqueue, &tasks[1]);
    ASSERT_EQ(osg.runqueue->nrp, &tasks[0]);
    ASSERT_EQ(osg.running, &tasks[1]);
    ASSERT_EQ(osg.scheduled, nullptr);
    ASSERT_EQ(osg.tasks, &tasks[1]);
    ASSERT_EQ(osg.tasks->np, &tasks[0]);

    ASSERT_EQ(tasks[0].status, OS_TASK_STATUS_IDLE);
    ASSERT_EQ(tasks[1].status, OS_TASK_STATUS_IDLE);

    ASSERT_EQ(tasks[0].priority, OS_PRIORITY_IDLE);
    ASSERT_EQ(tasks[1].priority, OS_PRIORITY_NORMAL);
}

static void task_handler_idle(void *p) {
}

static void task_handler_test(void *p) {
}

void PrintTo(os_task_t *task, std::ostream* os) {
    if (task == NULL) {
        *os << "<null>";
    }
    else {
        *os << "T<'" << task->name << "'>";
    }
}
