#pragma once

#include <stddef.h>

void serviceCompetitionMetrics();
void recordCompetitionApiRequest(size_t responseBytes = 0);
void recordCompetitionExternalRequest(size_t responseBytes = 0);
