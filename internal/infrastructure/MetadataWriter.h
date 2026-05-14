#pragma once

#include <string>
#include <vector>

#include "domain/SessionMetadata.h"
#include "domain/StreamStats.h"

namespace micecam::infrastructure {

class MetadataWriter {
public:
    static bool write_session_header(const domain::SessionMetadata& meta, const std::string& path);
    static bool write_session_footer(const std::string& path, uint64_t total_frames, uint64_t total_bytes, const std::string& session_checksum);
    static bool write_stats(const std::string& path, const std::vector<domain::StreamStats>& stats);
};

} // namespace micecam::infrastructure
