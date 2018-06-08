#include "../../Interface.hpp"
#include "../../InstEncoder.hpp"
#include "../../../BitProcessor.hpp"
#include "../FieldsPstG12.hpp"
#include "../../../../Frontend/IRToString.hpp"

#include <algorithm>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

using namespace iga;

// just so the object file won't whine if this module is not included
int dummy_symbol;

typedef std::function<void(uint64_t bits, std::stringstream &ss)> DecoderFunction;


