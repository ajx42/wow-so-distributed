#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

class FHManager
{
public:
  static FHManager& Instance()
  {
    static FHManager fhmanager;
    return fhmanager;
  }
  // Insert mapping, return error if already exists.
  int32_t insertFHMapping(int32_t fh, std::string path)
  {
    auto it = _lookupMapping(fh);

    if (it == fh_mapping.end())
    {
      fh_mapping.emplace_back(fh, path);
      return 0;
    }
    else
    {
      return -1;
    }
  }
  // Remove mapping, return error if fh not present.
  int32_t removeFHMapping(int32_t fh)
  {
    auto it = _lookupMapping(fh);

    if (it != fh_mapping.end())
    {
      fh_mapping.erase(it);
      return 0;
    }
    else
    {
      return -1;
    }
  }

  // Update existing mapping, return error if fh not present.
  int32_t updateFHMapping(int32_t fh, std::string path)
  {
    auto it = _lookupMapping(fh);

    if (it != fh_mapping.end())
    {
      it->second = path;
      return 0;
    }
    else
    {
      return -1;
    }
  }

  std::string lookupPath(int32_t fh)
  {
    return _lookupMapping(fh)->second;
  }

private:
  std::vector<std::pair<int32_t, std::string>> fh_mapping;
  std::vector<std::pair<int32_t, std::string>>::iterator _lookupMapping(int32_t fh)
  {
    return std::find_if(fh_mapping.begin(), fh_mapping.end(),
                        [fh](const std::pair<int32_t, std::string> &p)
                        {
                          return p.first == fh;
                        });
  }
};


