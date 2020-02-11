// TF_Games_Util++.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <fstream>

using namespace std;

namespace nusstudios {
	namespace core {
		namespace arraykit {
			template <typename T>
			size_t init(T** array) {
				*array = (T*)malloc(sizeof(T) * 3);
				return 3;
			}

			template <typename T>
			void init(T** array, size_t cap) {
				*array = (T*)malloc(sizeof(T) * cap);
			}

			template <typename T>
			void put(T** array, T element, size_t& index, size_t& capacity) {
				if (index == capacity)
					*array = (T*)realloc(*array, sizeof(T) * (capacity *= 2));
				(*array)[index++] = element;
			}

			template <typename T>
			void put(T** array, T* elements, size_t len, size_t& index, size_t& capacity) {
				for (size_t i = 0; i < len; i++)
					put(array, elements[i], index, capacity);
			}

			template <typename T>
			void end(T** array, size_t length) {
				*array = (T*)realloc(*array, sizeof(T) * length);
			}

			template <typename T>
			void reverse(T* array, size_t len) {
				for (size_t i = 0; i < (len / 2); i++)
					swap(array[i], array[len - (i + 1)]);
			}
		}

		namespace io {
			uint16_t ui16(char* buff, bool le) {
				uint16_t var = 0;
				for (char i = 0; i < 2; i++)
					var |= (((uint16_t)(unsigned char)buff[i]) << ((le ? i : (1 - i)) * 8));
				return var;
			}

			uint32_t ui32(char* buff, bool le) {
				uint32_t var = 0;
				for (char i = 0; i < 4; i++)
					var |= (((uint32_t)(unsigned char)buff[i]) << ((le ? i : (3 - i)) * 8));
				return var;
			}

			int32_t i32(char* buff, bool le) {
				return (int32_t)ui32(buff, le);
			}

			void fromui32(uint32_t from, char* to, bool le) {
				for (char i = 0; i < 4; i++)
					to[i] = (char)(from >> (le ? i : (3 - i)) * 8);
			}

			void fromi32(int32_t from, char* to, bool le) {
				fromui32(from, to, le);
			}

			void fromui16(uint16_t from, char* to, bool le) {
				for (char i = 0; i < 2; i++)
					to[i] = (char)(from >> (le ? i : (1 - i)) * 8);
			}

			size_t ifsz(ifstream &file) {
				file.ignore(std::numeric_limits<std::streamsize>::max());
				std::streamsize length = file.gcount();
				file.clear();   //  Since ignore will have set eof.
				file.seekg(0, ios::beg);
				return length;
			}
		}

		namespace parsing::utf {
			void encode2utf16(uint32_t codepoint, char** buff, char& len, bool le) {
				if (codepoint < 0x10000) {
					len = 2;
					*buff = (char*)calloc(2, sizeof(char));
					io::fromui16((uint16_t)codepoint, *buff, le);
				}
				else
				{
					len = 4;
					uint32_t cp = codepoint - 0x10000;
					uint16_t hs = 0xD800;
					uint16_t ls = 0xDC00;
					hs |= (uint16_t)(cp >> 10);
					ls |= (uint16_t)(cp & 0x3FF);
					char hs_buff[2] = { 0 };
					io::fromui16(hs, hs_buff, true);
					char ls_buff[2] = { 0 };
					io::fromui16(ls, ls_buff, true);
					*buff = (char*)malloc(sizeof(char) * 4);
					memcpy(*buff, hs_buff, sizeof(char) * 2);
					memcpy(*buff + 2, ls_buff, sizeof(char) * 2);
					free(hs_buff);
					free(ls_buff);

					if (!le)
						arraykit::reverse(*buff, 4);
				}
			}

			void w1252toutf16(char c, char* buff, bool le) {
				io::fromui16((uint16_t)(unsigned char)c, buff, le);
			}

			void asciitoutf16(char c, char* buff, bool le) {
				w1252toutf16(c, buff, le);
			}

			char* asciitoutf16(const char* c, bool le) {
				size_t bi = 0;
				size_t cap = 6;
				char* buff;
				arraykit::init(&buff, cap);

				for (size_t i = 0; c[i] != '\0'; i++) {
					char tmp[2];
					asciitoutf16(c[i], tmp, le);
					arraykit::put(&buff, tmp, 2, bi, cap);
				}

				arraykit::end(&buff, bi);
				return buff;
			}

			char* asciitoutf16(const char* c, size_t& len, bool le) {
				size_t bi = 0;
				size_t cap = 6;
				char* buff;
				arraykit::init(&buff, cap);

				for (size_t i = 0; c[i] != '\0'; i++) {
					char tmp[2];
					asciitoutf16(c[i], tmp, le);
					arraykit::put(&buff, tmp, 2, bi, cap);
				}

				len = bi;
				arraykit::end(&buff, bi);
				return buff;
			}

			// unused
			bool highsurrogate(uint16_t val) {
				return val >= 0xD800 && val <= 0xDBFF;
			}

			uint32_t decodeutf16(char* buff, size_t position, char& len, bool le) {
				char sub[2];
				memcpy(sub, buff + position, 2 * sizeof(char));
				position += 2;
				uint16_t first = io::ui16(sub, le);

				if ((first < 0xD800) ^ (first > 0xDFFF)) {
					len = 2;
					return first;
				}
				else if (first >= 0xD800 && first <= 0xDBFF)
				{
					len = 4;
					memcpy(sub, buff + position, sizeof(char) * 2);
					position += 2;
					uint16_t last = io::ui16(sub, le);

					if (last >= 0xDC00 && last <= 0xDFFF)
					{
						first = (uint16_t)(first & 0x3FF);
						last = (uint16_t)(last & 0x3FF);
						return ((uint32_t)first << 10) | ((uint32_t)last) + 0x10000;
					}
					else throw 3;
				}
				else throw 2;
			}

			char utf16toascii(char* buff, size_t position, bool le) {
				char len;
				return (char)decodeutf16(buff, position, len, le);
			}

			char utf16tow1252(char* buff, size_t position, bool le) {
				return utf16toascii(buff, position, le);
			}
		}

		namespace strkit {
			size_t* pfx_tbl(char* p, size_t plen) {
				size_t* tbl = (size_t*)malloc(sizeof(size_t) * plen);
				size_t i = 1;
				size_t lps = 0;
				tbl[0] = lps;

				while (i < plen) {
					if (p[i] == p[lps])
						tbl[i++] = ++lps;
					else
						if (lps == 0)
							tbl[i++] = 0;
						else
							lps = tbl[lps - 1];
				}

				return tbl;
			}

			size_t kmpfi(char* s, size_t slen, char* p, size_t plen, size_t* tbl) {
				size_t from = 0;

				for (size_t si = 0, pi = 0; si < slen;) {
					if (s[si] == p[pi]) {
						if (pi + 1 == plen)
							return from;
						else {
							si++;
							pi++;
						}
					}
					else {
						if (pi == 0)
							from = ++si;
						else {
							pi = tbl[pi - 1];
							from = si - pi;
						}
					}
				}
			}
		}
	}

	using namespace nusstudios::core;
	using namespace nusstudios::core::parsing;
	
	namespace tfutil {
		const char* key = "as;dwepo2345098]qw]{}p2039458pseasdfzcvvp;aseiurwefsdcfszdcvn";

		struct block
		{
			char* buff;
			uint32_t len;
			bool utf16;
		};

		void encrypt(const char* ipath, const char* opath, bool le) {
			ifstream infile;
			infile.open(ipath, ios::in | ios::binary);
			size_t len = io::ifsz(infile);
			char* fbuff = (char*)malloc(sizeof(char) * len);
			infile.read(fbuff, len);

			bool isu16 = false;
			size_t pos = 0;

			if (fbuff[0] != 'W') {
				isu16 = true;
				pos += 2;
			}
			else
				pos++;

			char *sep;
			size_t seplen;

			if (isu16)
				sep = utf::asciitoutf16("ENDBLOCK", seplen, true);
			else {
				seplen = strlen("ENDBLOCK");
				sep = (char *)malloc(sizeof(char) * seplen);
				memcpy(sep, "ENDBLOCK", sizeof(char) * seplen);
			}

			// building prefix table for KMP
			size_t* tbl = strkit::pfx_tbl(sep, seplen);
			block* r;
			size_t cap = arraykit::init(&r);
			size_t ri = 0;
			size_t cryptpos = 0;

			while (pos != len) {
				block b;

				if (fbuff[pos] != 'W')
					b.utf16 = true;
				else
					b.utf16 = false;

				// skipping encoding mark
				pos += isu16 ? 2 : 1;
				// Knuth-Morris-Pratt pattern matching based first index function
				size_t end = strkit::kmpfi(fbuff + pos, len - pos, sep, seplen, tbl) + pos;
				size_t blen = end - pos;
				char *fsub = (char*)malloc(sizeof(char) * blen);
				memcpy(fsub, fbuff + pos, blen);
				pos += seplen;

				if (blen == 0) {
					b.len = 0;
					b.buff = nullptr;
				}
				else {
					if (isu16) {
						if (!b.utf16) {
							for (size_t x = 0; x < blen / 2; x++)
								fsub[x] = utf::utf16tow1252(fsub, x * 2, true) ^ key[cryptpos++ % 61];
							fsub = (char*)realloc(fsub, blen / 2);
							b.len = blen / 2;
						}
						else {
							for (size_t x = 0; x < blen; x += 2)
								fsub[x] = fsub[x] ^ key[cryptpos++ % 61];
							b.len = blen;
						}
					}
					else {
						for (size_t x = 0; x < blen; x++)
							fsub[x] = fsub[x] ^ key[cryptpos++ % 61];
						b.len = blen;
					}

					pos += blen;
					b.buff = fsub;
				}

				arraykit::put(&r, b, ri, cap);
			}

			infile.close();
			arraykit::end(&r, ri);
			free(tbl);
			free(sep);
			free(fbuff);
			ofstream outfile;
			outfile.open(opath, ios::out | ios::binary);

			char numblockbuff[4];
			io::fromui32(ri, numblockbuff, le);
			outfile.write(numblockbuff, 4);

			for (size_t i = 0; i < ri; i++) {
				block& b = r[i];
				char blockszbuff[4];

				if (b.len != 0) {
					if (b.utf16)
						io::fromi32(((int32_t)(b.len + 2)) / -2, blockszbuff, le);
					else
						io::fromi32(b.len + 1, blockszbuff, le);
				}
				else
					memset(blockszbuff, 0, sizeof(char) * 4);

				outfile.write(blockszbuff, 4);

				if (b.len != 0) {
					outfile.write(b.buff, b.len);
					free(b.buff);

					if (b.utf16)
						outfile.write("\0\0", 2);
					else
						outfile.write("\0", 1);
				}
			}

			outfile.flush();
			outfile.close();
			free(r);
		}

		void decrypt(const char* ipath, const char* opath, bool le) {
			ifstream infile;
			infile.open(ipath, ios::in | ios::binary);

			char numblockbuff[4] = { 0 };
			infile.read(numblockbuff, 4);
			uint32_t numblock = io::ui32(numblockbuff, le);

			block* result = (block*)malloc(sizeof(block) * numblock);
			size_t cryptpos = 0;
			bool has16bitchars = false;

			for (uint32_t i = 0; i < numblock; i++) {
				char blockszbuff[4] = { 0 };
				infile.read(blockszbuff, 4);
				int32_t blocksz = io::i32(blockszbuff, le);

				char* buff;
				bool curr16bit = false;
				uint32_t len;

				if (blocksz < 0) {
					// UTF-16 encoding is used, but only the basic multilingual plain is used, so every char will be 2-byte long
					len = blocksz * -2;
					curr16bit = true;
					has16bitchars = true;
					buff = (char*)malloc(sizeof(char) * len);
					infile.read(buff, len);

					for (size_t x = 0; x < len - 2; x += 2)
						buff[x] = buff[x] ^ key[cryptpos++ % 61];
				}
				else if (blocksz == 0) {
					len = 0;
					buff = nullptr;
					curr16bit = false;
				}
				else {
					len = blocksz;
					buff = (char*)malloc(sizeof(char) * len);
					curr16bit = false;
					infile.read(buff, len);

					for (size_t x = 0; x < len - 1; x++)
						buff[x] = buff[x] ^ key[cryptpos++ % 61];
				}

				block b = { buff, len, curr16bit };
				memcpy(result + i, &b, sizeof(block));
			}

			infile.close();
			arraykit::end(&result, numblock);
			ofstream outfile;
			outfile.open(opath, ios::out | ios::binary);

			if (has16bitchars) {
				char buff[2];
				utf::asciitoutf16('U', buff, true);
				outfile.write(buff, 2);
			}
			else
				outfile.write("W", 1); // internal encoding is set to ASCII, so it is a subset of Windows-1252


			size_t seplen;
			char* sep = utf::asciitoutf16("ENDBLOCK", seplen, true);

			for (size_t i = 0; i < numblock; i++) {
				block& b = result[i];

				if (has16bitchars) {
					char markbuff[2];

					if (!b.utf16) {
						utf::asciitoutf16('W', markbuff, true);
						outfile.write(markbuff, 2);

						// last char is null '\0' on 1 byte so we omit it because it is only relevant for parsed data processing
						if (b.len != 0) {
							for (size_t x = 0; x < b.len - 1; x++) {
								char utf16char[2];
								utf::w1252toutf16(b.buff[x], utf16char, true);
								outfile.write(utf16char, 2);
							}
						}
					}
					else {
						utf::asciitoutf16('U', markbuff, true);
						outfile.write(markbuff, 2);
						outfile.write(b.buff, b.len - 2); // omitting last '\0' on 2 bytes
					}

					outfile.write(sep, seplen);
				}
				else {
					outfile.write("W", 1);
					outfile.write(b.buff, b.len - 1);
					outfile.write("ENDBLOCK", 8);
				}
			}

			outfile.flush();
			outfile.close();
			free(sep);
			free(result);
		}
	}
}

using namespace nusstudios;

int main()
{
	cout << "TF Games Util final from Patrik Nusszer in C++" << endl << endl;
	cout << "Select encryption or decryption (E/D): ";
	char selection;
	cin >> selection;

	cout << endl << "Enter endainess ('l' for Windows, 'b' for consoles): ";
	char endian;
	cin >> endian;
	bool le = true;

	if (endian == 'l' || endian == 'L')
		le = true;
	else if (endian == 'b' || endian == 'B')
		le = false;

	char inp[260];
	cout << endl << "Enter input file path: ";
	cin.ignore();
	cin.getline(inp, 260);
	char outp[260];
	cout << endl << "Enter output file path: ";
	cin.getline(outp, 260);

	if (selection == 'd' || selection == 'D') {
		tfutil::decrypt(inp, outp, le);
		cout << endl << "Decryption finished";
	}
	else if (selection == 'e' || selection == 'E') {
		tfutil::encrypt(inp, outp, le);
		cout << endl << "Encryption finished";
	}
	else return 1;

	return 0;
}