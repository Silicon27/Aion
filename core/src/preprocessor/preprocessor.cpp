//
// Created by David Yang on 2025-10-18.
//

#include <preprocessor/preprocessor.hpp>

Preprocessor::Preprocessor(std::string file) : file(file, std::ios::in) {

}

Preprocessor::~Preprocessor() = default;



std::string Preprocessor::get_next_line_with_preprocessing_directive() {
    // while (file.getline)
    return "";
}

void Preprocessor::preprocess() {
    // Stub implementation for now.
}

