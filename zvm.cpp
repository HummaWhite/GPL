#include <iostream>
#include <fstream>
#include <sstream>
#include <cstring>
#include <iomanip>
#include <vector>
#include <map>

const int MEM_SIZE = 65536;
const int REG_SIZE = 16;
const int END_EXEC = -1;

enum Flags
{
	SF = 1 << 0,
	ZF = 1 << 1,
	CF = 1 << 2,
	OF = 1 << 3
};

enum InstrLength
{
	LenInstrNoReg = 1,
	LenInstrMonoReg = 1,
	LenInstrDuoReg = 2,
	LenInstrTriReg = 2,
	LenInstrQuadReg = 3,
	LenInstrImm = 5
};

typedef std::pair<std::string, int32_t> InstrInfo;

const InstrInfo INSTRUCTIONS[] =
{
	{ "noop", LenInstrNoReg },		// 0x00
	{ "movr", LenInstrDuoReg },		// 0x01
	{ "notl", LenInstrDuoReg },		// 0x02
	{ "nega", LenInstrDuoReg },		// 0x03
	{ "muls", LenInstrQuadReg },	// 0x04
	{ "divs", LenInstrQuadReg },	// 0x05
	{ "comp", LenInstrDuoReg },		// 0x06
	{ "jmpi", LenInstrImm },		// 0x07
	{ "cali", LenInstrImm },		// 0x08
	{ "retn", LenInstrNoReg },		// 0x09
	{ "jpgt", LenInstrImm },		// 0x0A
	{ "jpls", LenInstrImm },		// 0x0B
	{ "jpge", LenInstrImm },		// 0x0C
	{ "jple", LenInstrImm },		// 0x0D
	{ "jpeq", LenInstrImm },		// 0x0E
	{ "jpne", LenInstrImm },		// 0x0F
	{ "movi", LenInstrImm },		// 0x10
	{ "stor", LenInstrTriReg },		// 0x20
	{ "load", LenInstrTriReg },		// 0x30
	{ "adds", LenInstrTriReg }, 	// 0x40
	{ "subs", LenInstrTriReg }, 	// 0x50
	{ "andl", LenInstrTriReg }, 	// 0x60
	{ "orll", LenInstrTriReg }, 	// 0x70
	{ "xorl", LenInstrTriReg }, 	// 0x80
	{ "shll", LenInstrTriReg }, 	// 0x90
	{ "shrl", LenInstrTriReg }, 	// 0xA0
	{ "shra", LenInstrTriReg }, 	// 0xB0
	{ "jmpr", LenInstrMonoReg }, 	// 0xC0
	{ "calr", LenInstrMonoReg },	// 0xD0
	{ "halt", LenInstrNoReg }		// 0xE0
};

uint8_t mem[MEM_SIZE];
int32_t reg[REG_SIZE];
uint8_t fl = 0;

bool debug = true;
bool enableCmd = false;

std::stringstream debugInfo;

void setFlag(Flags flag, bool v)
{
	fl ^= (fl & flag);
	if (v) fl |= flag;
}

bool queryFlag(Flags flag)
{
	return fl & flag;
}

std::tuple<uint8_t, uint8_t> decodeInstrDuoReg(int32_t addr)
{
	return { mem[addr + 1] >> 4, mem[addr + 1] & 0xF };
}

std::tuple<uint8_t, uint8_t, uint8_t> decodeInstrTriReg(int32_t addr)
{
	return { mem[addr] & 0xF, mem[addr + 1] >> 4, mem[addr + 1] & 0xF };
}

std::tuple<uint8_t, uint8_t, uint8_t, uint8_t> decodeInstrQuadReg(int32_t addr)
{
	return { mem[addr + 1] >> 4, mem[addr + 1] & 0xF, mem[addr + 2] >> 4, mem[addr + 2] & 0xF };
}

std::tuple<uint8_t, int32_t> decodeInstrRegImm(int32_t addr)
{
	return { mem[addr] & 0xF, *(int32_t*)(mem + addr + 1) };
}

int32_t decodeInstrJmpImm(int32_t addr)
{
	return *(int32_t*)(mem + addr + 1);
}

std::vector<uint8_t> fetchInstrData(int32_t addr)
{
	uint8_t instrIndex = (mem[addr] & 0xF0) ? (15 + (mem[addr] >> 4)) : mem[addr] & 0xF;
	int instrLen = INSTRUCTIONS[instrIndex].second;
	std::vector<uint8_t> instr(instrLen);
	std::memcpy(instr.data(), mem + addr, instrLen);
	return instr;
}

std::string dataToStr(int32_t addr, int size, int newLineLim = 0)
{
	std::stringstream ret;
	for (int i = 0; i < size; i++)
	{
		ret << std::setw(2) << std::setfill('0') << std::hex << int(mem[addr + i]);
		if (newLineLim)
		{
			if (i % newLineLim == newLineLim - 1) ret << "\n";
			else ret << " ";
		}
		else ret << " ";
	}
	return ret.str();
}

std::string instrDataToStr(int32_t addr)
{
	std::stringstream ret;
	auto fetched = fetchInstrData(addr);
	for (auto byte : fetched)
	{
		ret << std::setw(2) << std::setfill('0') << std::hex << int(byte) << " ";
	}
	return ret.str();
}

std::string regToStr(uint8_t index)
{
	return "r" + std::to_string(int(index));
}

int32_t noop(int32_t ip)
{
	debugInfo << "noop";
	return ip + LenInstrNoReg;
}

int32_t movr(int32_t ip)
{
	auto [rd, rs] = decodeInstrDuoReg(ip);
	reg[rd] = reg[rs];
	debugInfo << "movr " << regToStr(rd) << ", " << regToStr(rs);
	return ip + LenInstrMonoReg;
}

int32_t movi(int32_t ip)
{
	auto [rd, imm] = decodeInstrRegImm(ip);
	reg[rd] = imm;
	debugInfo << "movi " << regToStr(rd) << ", 0x" << std::hex << imm;
	return ip + LenInstrImm;
}

int32_t stor(int32_t ip)
{
	auto [rs, rb, ro] = decodeInstrTriReg(ip);
	*(int32_t*)(mem + reg[rb] + reg[ro]) = reg[rs];
	debugInfo << "stor [" << regToStr(rb) << " + " << regToStr(ro) << "], " << regToStr(rs);
	return ip + LenInstrTriReg;
}

int32_t load(int32_t ip)
{
	auto [rd, rb, ro] = decodeInstrTriReg(ip);
	reg[rd] = *(int32_t*)(mem + reg[rb] + reg[ro]);
	debugInfo << "load " << regToStr(rd) << ", [" << regToStr(rb) << " + " << regToStr(ro) << "]";
	return ip + LenInstrTriReg;
}

int32_t adds(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] + reg[rt];
	debugInfo << "adds " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t subs(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] - reg[rt];
	debugInfo << "subs " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t andl(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] & reg[rt];
	debugInfo << "andl " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t orll(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] | reg[rt];
	debugInfo << "orll " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t xorl(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] ^ reg[rt];
	debugInfo << "xorl " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t notl(int32_t ip)
{
	auto [rd, rs] = decodeInstrDuoReg(ip);
	reg[rd] = ~reg[rs];
	debugInfo << "notl " << regToStr(rd) << ", " << regToStr(rs);
	return ip + LenInstrDuoReg;
}

int32_t nega(int32_t ip)
{
	auto [rd, rs] = decodeInstrDuoReg(ip);
	reg[rd] = -reg[rs];
	debugInfo << "nega " << regToStr(rd) << ", " << regToStr(rs);
	return ip + LenInstrDuoReg;
}

int32_t shll(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] << *(uint32_t*)(reg + rt);
	debugInfo << "shll " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t shrl(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	*(uint32_t*)(reg + rd) = *(uint32_t*)(reg + rs) >> *(uint32_t*)(reg + rt);
	debugInfo << "shrl " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t shra(int32_t ip)
{
	auto [rd, rs, rt] = decodeInstrTriReg(ip);
	reg[rd] = reg[rs] >> *(uint32_t*)(reg + rt);
	debugInfo << "shra " << regToStr(rd) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrTriReg;
}

int32_t muls(int32_t ip)
{
	auto [rh, rl, rs, rt] = decodeInstrQuadReg(ip);
	int64_t tmp = int64_t(reg[rs]) * int64_t(reg[rt]);
	reg[rh] = tmp >> 32;
	reg[rl] = tmp & 0x00000000FFFFFFFFll;
	debugInfo << "muls " << regToStr(rh) << ", " << regToStr(rl) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrQuadReg;
}

int32_t divs(int32_t ip)
{
	auto [rd, rr, rs, rt] = decodeInstrQuadReg(ip);
	reg[rd] = reg[rs] / reg[rt];
	reg[rr] = reg[rs] % reg[rt];
	debugInfo << "divs " << regToStr(rd) << ", " << regToStr(rr) << ", " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrQuadReg;
}

int32_t comp(int32_t ip)
{
	auto [rs, rt] = decodeInstrDuoReg(ip);
	int res = reg[rs] - reg[rt];
	setFlag(SF, res & 0x80000000);
	setFlag(ZF, res);
	debugInfo << "comp " << regToStr(rs) << ", " << regToStr(rt);
	return ip + LenInstrDuoReg;
}

int32_t jmpi(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jmpi 0x" << std::hex << addr;
	return addr;
}

int32_t jmpr(int32_t ip)
{
	uint8_t rs = mem[ip] & 0xF;
	debugInfo << "jmpr " << regToStr(rs);
	return reg[rs];
}

int32_t cali(int32_t ip)
{
	return ip;
}

int32_t calr(int32_t ip)
{
	return ip;
}

int32_t retn(int32_t ip)
{
	return ip;
}

int32_t jpgt(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jpgt 0x" << std::hex << addr;
	if (!queryFlag(SF) && queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t jpls(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jpls 0x" << std::hex << addr;
	if (queryFlag(SF) && queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t jpge(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jpge 0x" << std::hex << addr;
	if (!queryFlag(SF) || !queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t jple(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jple 0x" << std::hex << addr;
	if (queryFlag(SF) || !queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t jpeq(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jpeq 0x" << std::hex << addr;
	if (queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t jpne(int32_t ip)
{
	auto addr = decodeInstrJmpImm(ip);
	debugInfo << "jpne 0x" << std::hex << addr;
	if (!queryFlag(ZF)) return addr;
	return ip + LenInstrImm;
}

int32_t halt(int32_t ip)
{
	debugInfo << "halt";
	return END_EXEC;
}

int32_t (*instrFunc[])(int32_t) =
{
	noop, movr, notl, nega, muls, divs, comp, jmpi, cali, retn, jpgt, jpls, jpge, jple, jpeq, jpne,
	movi, stor, load, adds, subs, andl, orll, xorl, shll, shrl, shra, jmpr, calr, halt
};

int32_t execute(int32_t ip)
{
	uint8_t instrIndex = (mem[ip] & 0xF0) ? (15 + (mem[ip] >> 4)) : mem[ip] & 0xF;
	return instrFunc[instrIndex](ip);
}

enum class CommandType
{
	Exec, Suspend, End
};

CommandType parseCommand(const std::string &cmd)
{
	if (cmd.length() == 0) return CommandType::Suspend;

	std::stringstream ss(cmd);
	std::string op;
	ss >> op;

	if (op == "s") return CommandType::Exec;
	if (op == "q") return CommandType::End;

	if (op == "p")
	{
		std::string arg;
		ss >> arg;
		if (std::isdigit(arg[0]))
		{
			int addr = std::stoi(arg);
			std::string size;
			ss >> size;
			std::cout << dataToStr(addr, std::stoi(size), 16) << "\n";
		}
		else
		{
			int index = std::stoi(arg.c_str() + 1);
			std::cout << "0x" << std::setw(8) << std::setfill('0') << std::hex << reg[index] << "\n";
		}
		return CommandType::Suspend;
	}

	if (op == "r")
	{
		for (int i = 0; i < REG_SIZE; i++)
		{
			std::cout << regToStr(i) << ":\t";
			std::cout << "0x" << std::setw(8) << std::setfill('0') << std::hex << reg[i] << "\n";
		}
		return CommandType::Suspend;
	}
	return CommandType::Suspend;
}

const uint8_t testInstr[] =
{
	0x00,							// noop
	0x10, 0x12, 0x34, 0x56, 0x78,	// movi r0, 0x78563412
	0x11, 0x87, 0x65, 0x43, 0x21,	// movi r1, 0x21436587
	0x82, 0x01,						// xorl r2, r0, r1
	0x13, 0x04, 0x00, 0x00, 0x00,	// movi r3, 0x00000004
	0x14, 0x00, 0x00, 0x00, 0x00,	// movi r4, 0x00000000
	0x15, 0x01, 0x00, 0x00, 0x00,	// movi r5, 0x00000001
	0x53, 0x35,						// subs r3, r3, r5
	0x06, 0x34,						// comp r3, r4
	0x0A, 0x1C, 0x00, 0x00, 0x00,	// jpgt 0x0000001C
	0x04, 0x32, 0x10,				// muls r3, r2, r1, r0
	0xE0							// halt
};

int main(int argc, char *argv[])
{
	int32_t ip = 0;
	if (argc == 2)
	{
		std::string arg(argv[1]);
		enableCmd = (arg == "-d");
	}

	std::memcpy(mem, testInstr, sizeof(testInstr));
	std::memset(reg, 0, sizeof(reg));

	while (ip != END_EXEC)
	{
		if (enableCmd)
		{
			std::string cmd;
			std::cout << "Debug>> ";
			std::getline(std::cin, cmd);

			auto res = parseCommand(cmd);
			if (res == CommandType::Suspend) continue;
			if (res == CommandType::End) break;
		}

		std::stringstream ss;
		ss << "0x" << std::setw(8) << std::setfill('0') << std::hex << ip << "  ";
		ss << std::setw(16) << std::setfill(' ') << instrDataToStr(ip) << "\t";

		ip = execute(ip);

		if (debug) ss << debugInfo.str();
		std::cout << ss.str() << "\n";
		debugInfo.str("");
	}
}
