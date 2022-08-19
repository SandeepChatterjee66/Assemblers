#ifndef UTILS_H // include guard
#define UTILS_H

std::string toString(int i);

std::string toHex(int i);

std::string toHex(int i, int bytes);

std::string toBin(std::string hex);

bool replace(std::string& str, const std::string& from, const std::string& to);
