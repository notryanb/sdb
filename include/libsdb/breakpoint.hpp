#ifndef SDB_BREAKPOINT_HPP
#define SDB_BREAKPOINT_HPP

#include <cstdint>
#include <cstddef>
#include <string>
#include <libsdb/stoppoint_collection>
#include <libsdb/breakpoint_site>
#include <libsdb/types.hpp>

namespace sdb {
  class target;
  class breakpoint {
    public:
      virtual ~breakpoint() = default;

      breakpoint() = delete;
      breakpoint(const breakpoint&) = delete;
      breakpoint& operator=(const breakpoint&) = delete;

      using id_type = std::int32_t;
      id_type id() const { return id_; }

      void enable();
      void disable();

      bool is_enabled() const { return is_enabled_; }
      bool is_hardware() const { return is_hardware_; }
      bool is_internal() const { return is_internal_; }

      virtual void resolve() = 0;

      /*?*/ breakpoint_sites() { return breakpoint_sites_; }
      
    protected:
      friend target;

      breakpoint(
        target& tgt,
        bool is_hardware = false,
        bool is_internal = false
      );

    id_type id_;
    target* target_;
    bool is_enabled_ = false;
    bool is_hardware_ = false;
    bool is_internal_ = false;
    /*?*/ breakpoint_sites_;
    breakpoint_site::id_type next_site_id_ = 1;
  };
}

#endif
