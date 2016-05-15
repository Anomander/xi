#pragma once

#include <boost/intrusive/list.hpp>
#include <boost/intrusive/set.hpp>

namespace xi {
inline namespace ext {
  namespace intrusive {
    using ::boost::intrusive::list_base_hook;
    using ::boost::intrusive::list_member_hook;
    using ::boost::intrusive::set_member_hook;
    using ::boost::intrusive::member_hook;
    using ::boost::intrusive::list;
    using ::boost::intrusive::set;
    using ::boost::intrusive::constant_time_size;
    using ::boost::intrusive::link_mode;
    using ::boost::intrusive::normal_link;
    using ::boost::intrusive::safe_link;
    using ::boost::intrusive::auto_unlink;
    using ::boost::intrusive::compare;
  }
}
}
