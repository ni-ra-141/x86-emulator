// Unit tests for Task 2: instruction queue + fetcher.
#include "../../common/mini_test.hpp"
#include "../fetcher.hpp"
#include "../instruction_queue.hpp"
#include "../../task1_cpu_segmentation/emu_error.hpp"

using namespace emu;

TEST(queue_fifo_order) {
    InstructionQueue q(4);
    CHECK(q.empty());
    CHECK(q.enqueue(1)); CHECK(q.enqueue(2)); CHECK(q.enqueue(3));
    uint8_t b = 0;
    CHECK(q.dequeue(b)); CHECK_EQ(int(b), 1);
    CHECK(q.dequeue(b)); CHECK_EQ(int(b), 2);
    CHECK_EQ(q.size(), 1u);
}

TEST(queue_full_and_empty_behaviour) {
    InstructionQueue q(2);
    CHECK(q.enqueue(10)); CHECK(q.enqueue(20));
    CHECK(q.full());
    CHECK(!q.enqueue(30));        // rejected, not overwritten
    uint8_t b;
    CHECK(q.dequeue(b)); CHECK(q.dequeue(b));
    CHECK(!q.dequeue(b));         // empty
}

TEST(queue_wraps_around_circular_buffer) {
    InstructionQueue q(3);
    uint8_t b;
    for (int round = 0; round < 10; ++round) {
        q.enqueue(uint8_t(round)); q.enqueue(uint8_t(round + 100));
        CHECK(q.dequeue(b)); CHECK_EQ(int(b), round);
        CHECK(q.dequeue(b)); CHECK_EQ(int(b), round + 100);
    }
}

TEST(queue_peek_and_flush) {
    InstructionQueue q(4);
    q.enqueue(5); q.enqueue(6);
    CHECK_EQ(int(q.peek(0)), 5);
    CHECK_EQ(int(q.peek(1)), 6);
    CHECK_THROWS(q.peek(2));
    uint8_t out[4];
    CHECK_EQ(q.copyTo(out, 4), 2u);
    CHECK_EQ(q.size(), 2u);       // copyTo does not remove
    q.flush();
    CHECK(q.empty());
}

static void loadAt(CPUState& cpu, uint16_t cs, uint16_t ip, std::initializer_list<uint8_t> bytes) {
    cpu.regs.setSeg(SegReg::CS, cs);
    cpu.regs.ip = ip;
    std::vector<uint8_t> v(bytes);
    cpu.memory.loadBlock(Segmentation::physical(cs, ip), v.data(), v.size());
}

TEST(fetcher_fills_queue_from_cs_ip) {
    CPUState cpu;
    loadAt(cpu, 0x0100, 0x0000, {0xB8, 0x05, 0x00, 0x90});
    Fetcher f(cpu, 8);
    CHECK_EQ(f.fill(), 8u);
    uint8_t buf[8];
    CHECK_EQ(f.peek(buf, 8), 8u);
    CHECK_EQ(int(buf[0]), 0xB8); CHECK_EQ(int(buf[1]), 0x05); CHECK_EQ(int(buf[3]), 0x90);
    CHECK_EQ(cpu.regs.ip, 0);     // fetching does NOT change IP
}

TEST(fetcher_consume_advances_ip) {
    CPUState cpu;
    loadAt(cpu, 0x0100, 0x0000, {0xB8, 0x05, 0x00, 0x90});
    Fetcher f(cpu, 8);
    f.fill();
    f.consume(3);
    CHECK_EQ(cpu.regs.ip, 3);
    uint8_t buf[8];
    f.peek(buf, 8);
    CHECK_EQ(int(buf[0]), 0x90);  // next instruction now at queue head
    CHECK_EQ(f.queued(), 5u);
}

TEST(fetcher_consume_too_many_throws) {
    CPUState cpu;
    Fetcher f(cpu, 4);
    f.fetchOneByte();
    CHECK_THROWS(f.consume(2));
}

TEST(fetcher_auto_flush_after_jump) {
    CPUState cpu;
    loadAt(cpu, 0x0100, 0x0000, {0x90, 0x90, 0x90, 0x90});
    loadAt(cpu, 0x0100, 0x0040, {0xF4});          // HLT at the jump target
    cpu.regs.ip = 0;
    Fetcher f(cpu, 8);
    f.fill();
    cpu.regs.ip = 0x0040;                         // W2 executes a JMP
    uint8_t buf[8];
    f.peek(buf, 8);                               // queue silently refilled from new IP
    f.fill();
    f.peek(buf, 8);
    CHECK_EQ(int(buf[0]), 0xF4);
    CHECK_EQ(f.nextFetchOffset(), uint16_t(0x0048));
}

TEST(fetcher_ip_offset_wraps_at_64k) {
    CPUState cpu;
    loadAt(cpu, 0x1000, 0xFFFE, {0xAA, 0xBB});
    cpu.memory.write8(Segmentation::physical(0x1000, 0x0000), 0xCC);
    Fetcher f(cpu, 3);
    f.fill();
    uint8_t buf[3];
    f.peek(buf, 3);
    CHECK_EQ(int(buf[0]), 0xAA); CHECK_EQ(int(buf[1]), 0xBB); CHECK_EQ(int(buf[2]), 0xCC);
}

TEST(fetcher_single_byte_fetch_models_bus_cycles) {
    CPUState cpu;
    loadAt(cpu, 0, 0x100, {1, 2, 3});
    Fetcher f(cpu, 2);
    CHECK(f.fetchOneByte()); CHECK_EQ(f.queued(), 1u);
    CHECK(f.fetchOneByte()); CHECK_EQ(f.queued(), 2u);
    CHECK(!f.fetchOneByte());     // queue full -> BIU stalls
}

int main() { return run_all_tests(); }
