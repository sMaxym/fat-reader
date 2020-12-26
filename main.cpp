#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <iostream>
#include <fstream>
#include <map>

#pragma pack(push, 1)

struct fat16_bs_t {
	u_char 					bootjmp[3];
	u_char 					oem[8];
	u_short	 				sect_size;
	u_char 					sects_in_cluster;
	u_short 				rsrvd_sects;
	u_char 					fats_n;
	u_short 				root_files_n;
	u_short 				sect_n;
	u_char 					media;
	u_short 				fat_size;
	u_short 				sects_per_track;
	u_short 				heads_n;
	u_long 					hidden_sects_n;
	u_long 					total_sects_long;

	u_char 					drive;
	u_char 					current_head;
	u_char 					boot_signature;
	u_long 					volume_id;
	char 					volume_label[11];
	char 					fs_type[8];
	char 					boot_code[448];
	u_short	 				signature;
};

struct fat16_entry_t {
	u_char 		filename[8];
	u_char 		ext[3];
	u_char 		attributes;
	u_char 		reserved[10];
	u_short 	modify_time;
	u_short 	modify_date;
	u_short 	starting_cluster;
	int 		file_size;
};

#pragma pack(pop)

void print_fat16_info(const fat16_bs_t& fat16);
std::string uchar2string(const u_char* s, size_t size);
std::string date_format(u_short date, u_short time, char delim=':');

int main(int argc, char** argv) {
	const int 	fat16_name_size = 8,
				fat16_ext_size = 3;
	const int pad_header = 20;
	const std::map<std::string, u_char> attrs = {
			{"read-only", 0x01},
			{"hidden", 0x02},
			{"system", 0x04},
			{"label", 0x08},
			{"dir", 0x10},
			{"archive", 0x20}
	};

	std::string e_name, e_ext;
	std::ifstream fs;
	fat16_bs_t bs{};

	if (argc != 2)
	{
		std::cout << "[fat16reader] invalid amount of arguments" << std::endl;
		return 0;
	}

	fs.open(argv[1], std::ios::in | std::ios::binary);

	if (fs.fail())
	{
		std::cerr << "[fat16reader] cannot open image" << std::endl;
		return 0;
	}

	fs.read(reinterpret_cast<char *>(&bs), sizeof(bs));
	print_fat16_info(bs);

	size_t offset = (bs.rsrvd_sects + bs.fat_size * +bs.fats_n) * bs.sect_size;
	size_t files_n = +bs.root_files_n;
	fs.seekg(offset);

	std::cout	<< std::endl << std::right << std::setw(pad_header)
				<< "NAME" << std::setw(pad_header)
				<< "DATE&TIME" << std::setw(pad_header)
				<< "SIZE" << std::setw(pad_header)
				<< "ATTRS" << std::endl;
	for (size_t i = 0; i < files_n; ++i) {
		fat16_entry_t fat_entry{};
		fs.read(reinterpret_cast<char *>(&fat_entry), sizeof(fat16_entry_t));

		e_name = uchar2string(fat_entry.filename, fat16_name_size);
		e_ext = uchar2string(fat_entry.ext, fat16_ext_size);
		if (e_name.empty()) continue;

		if (!e_ext.empty()) e_name += "." + e_ext;
		std::cout << std::setw(pad_header) << e_name;
		std::cout << std::setw(pad_header) << date_format(fat_entry.modify_date,
															fat_entry.modify_time);
		std::cout << std::setw(pad_header) << fat_entry.file_size;
		for (auto const &[attr_str, attr] : attrs) {
			if ( attr == fat_entry.attributes )
				std::cout << std::setw(pad_header) << attr_str;
		}
		std::cout << std::endl;
	}

	fs.close();
	return 0;
}

void print_fat16_info(const fat16_bs_t& fat16) {
	const int pad_name = 23;
	const int pad_value = 10;
	std::map<std::string, std::string> info = {
			{"sector size", std::to_string(fat16.sect_size)},
			{"sectors number", std::to_string(+fat16.sects_in_cluster)},
			{"fats number", std::to_string(+fat16.fats_n)},
			{"fat size (sectors)", std::to_string(fat16.fat_size)},
			{"fat size (bytes)", std::to_string(fat16.fat_size * fat16.sect_size)},
			{"root entries", std::to_string(fat16.root_files_n)},
			{"root entries (bytes)", std::to_string(fat16.root_files_n * fat16.sect_size)},
			{"reserved sectors", std::to_string(fat16.rsrvd_sects)},
			{"signature", std::to_string(fat16.boot_signature)}
	};
	std::cout 	<< "FAT16 image info:\n" << std::right;
	for (const auto& [msg, val] : info)
		std::cout 	<< std::setw(pad_name) << msg
					<< std::setw(pad_value) << val << std::endl;
}

std::string uchar2string(const u_char* s, size_t size) {
	std::string str;
	std::find_if(s, s + size, [&str](const auto c){
		if (c == 0 || c == 32) return true;
		str += c;
		return false;
	});
	return str;
}

std::string date_format(u_short date, u_short time, char delim) {
	std::stringstream df;
	df 	<< 1980 + (date >> 0x9) << delim
		<< ((date >> 5) & 0xf) << delim
		<< (date & 0x1f) << " "
		<< (time & 0x1f) << delim
		<< ((time >> 0x5) & 0x3f) << delim
		<< (time >> 11);
	return df.str();
}