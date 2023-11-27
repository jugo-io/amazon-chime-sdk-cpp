//  Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
/* @unstable */

#ifndef CHIME_SIGNALING_STRING_UTILS_H_
#define CHIME_SIGNALING_STRING_UTILS_H_

#include "audio_video/media_section.h"

#include <array>
#include <algorithm>
#include <cstdio>
#include <optional>
#include <string>
#include <regex>
#include <string_view>

namespace chime {


const std::string rec_only = "a=recvonly";
const std::string send_only = "a=sendonly";
const std::string inactive = "a=inactive";
const std::string sendrecv = "a=sendrecv";
const std::string mid_prefix = "a=mid:";

class SDPUtils {
 public:
  static std::string RemoveFirstOccurrence(std::string& str, const std::string& word) {
    auto pos = str.find(word);
    if (pos == std::string::npos) return str;

    str.erase(pos, word.length());
    return str;
  }

  static std::string RemoveAllSinceWordOccurrence(std::string& str, const std::string& word) {
    auto pos = str.find(word);
    if (pos == std::string::npos) return str;
    str.erase(str.begin() + static_cast<int>(pos), str.end());
    return str;
  }

  static std::vector<std::string> Split(const std::string& str, const std::string& delimiter) {
    size_t pos_start = 0, pos_end, delim_len = delimiter.length();
    std::string token;
    std::vector<std::string> res;

    while ((pos_end = str.find(delimiter, pos_start)) != std::string::npos) {
      token = str.substr(pos_start, pos_end - pos_start);
      pos_start = pos_end + delim_len;
      res.push_back(token);
    }

    res.push_back(str.substr(pos_start));
    return res;
  }

  // TODO @hokyungh: might be worth if we don't do parsing by ourselves.
  static std::vector<MediaSection> ParseSDP(const std::string& sdp) {
    std::vector<std::string> sdp_lines = Split(sdp, "\r\n");
    std::vector<MediaSection> media_sections;
    std::string mid;

    auto is_media_section_header = [](std::string_view line) { return StartsWith(line, "m="); };

    const auto sdp_end = std::cend(sdp_lines);
    auto section_start = std::find_if(std::cbegin(sdp_lines), sdp_end, is_media_section_header);

    while (section_start != sdp_end) {
      auto section_end = std::find_if(section_start + 1, sdp_end, is_media_section_header);

      auto media_section = ParseMediaSection(section_start, section_end);

      if (media_section) {
        media_sections.push_back(std::move(media_section.value()));
      } else {
        // TODO: proper logging
        std::fprintf(stderr, "failed to parse media section starting with: %s\n", section_start->c_str());
      }

      section_start = section_end;
    }
    return media_sections;
  }

 private:
  static bool StartsWith(std::string_view str, std::string_view prefix) {
    return str.substr(0, prefix.length()) == prefix;
  }

  static std::optional<MediaDirection> GetDirection(const std::string_view str) {
    if (str == rec_only) return MediaDirection::kRecvOnly;
    if (str == inactive) return MediaDirection::kInactive;
    if (str == send_only) return MediaDirection::kSendOnly;
    if (str == sendrecv) return MediaDirection::kSendRecv;

    return {};
  }

  static std::optional<MediaSection> ParseMediaSection(std::vector<std::string>::const_iterator begin, const std::vector<std::string>::const_iterator end) {
    MediaType type = StartsWith(*begin, "m=audio") ? MediaType::kAudio : MediaType::kVideo;
    std::string mid;
    std::optional<MediaDirection> dir;

    while (begin != end) {
      if (StartsWith(*begin, mid_prefix)) {
        mid = begin->substr(mid_prefix.length());
          printf("found mid: %s\n", mid.c_str());
      } else if (!dir) {
        dir = GetDirection(*begin);
        if (dir) {
          printf("found direction: %d\n", *dir);
        }
      }
      ++begin;
    }

    if (mid.length() > 0 && dir) {
      return std::optional<MediaSection>{MediaSection{type, mid, *dir}};
    }

    return {};
  }
};

}  // namespace chime
#endif  // CHIME_SIGNALING_STRING_UTILS_H_
