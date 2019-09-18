#pragma once

void otaDebugProgress(unsigned int bytes);
void otaDebugError();

bool otaBegin();
bool otaWrite(uint8_t* data, size_t len);
bool otaEnd(size_t size, unsigned char reset_reason = 0);
