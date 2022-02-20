/* 
 * MVisor
 * Copyright (C) 2021 Terrence <terrence@tenclass.com>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "disk_image.h"
#include <unistd.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include "logger.h"
#include "device_manager.h"

class RawImage : public DiskImage {
 private:
  int fd_ = -1;
  size_t block_size_ = 512;
  size_t total_blocks_ = 0;

  ImageInformation information() {
    return ImageInformation {
      .block_size = block_size_,
      .total_blocks = total_blocks_
    };
  }

  virtual ~RawImage() {
    if (fd_ != -1) {
      Flush([=](long ret) {
        close(fd_);
      });
    }
  }

  void Initialize(const std::string& path, bool readonly) {
    readonly_ = readonly;

    if (readonly) {
      fd_ = open(path.c_str(), O_RDONLY);
    } else {
      fd_ = open(path.c_str(), O_RDWR);
    }
    if (fd_ < 0)
      MV_PANIC("disk file not found: %s", path.c_str());

    struct stat st;
    fstat(fd_, &st);
    block_size_ = 512;
    total_blocks_ = st.st_size / block_size_;
  }

  void Read(void *buffer, off_t position, size_t length, IoCallback callback) {
    auto io = device_->manager()->io();
    io->Read(fd_, buffer, length, position, callback);
  }

  void Write(void *buffer, off_t position, size_t length, IoCallback callback) {
    if (readonly_) {
      callback(0);
    } else {
      auto io = device_->manager()->io();
      io->Write(fd_, buffer, length, position, callback);
    }
  }

  void Flush(IoCallback callback) {
    if (readonly_) {
      callback(0);
    } else {
      auto io = device_->manager()->io();
      io->FSync(fd_, 0, callback);
    }
  }

};

DECLARE_DISK_IMAGE(RawImage);
