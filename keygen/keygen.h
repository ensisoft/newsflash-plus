
#pragma once

#include <QString>

namespace keygen
{

// Generate a machine based fingerprint
QString generate_fingerprint();

// Generate a matching unique keycode based on the fingerprint
QString generate_keycode(const QString& fingerprint);

// check that the given keycode is ok on this computer
bool verify_code(const QString& keycode);

} // keygen




