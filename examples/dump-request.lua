-- Copyright 2015 CBS Interactive Inc.
--
-- Licensed under the Apache License, Version 2.0 (the "License");
-- you may not use this file except in compliance with the License.
-- You may obtain a copy of the License at
--
--      http://www.apache.org/licenses/LICENSE-2.0
--
-- Unless required by applicable law or agreed to in writing, software
-- distributed under the License is distributed on an "AS IS" BASIS,
-- WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
-- See the License for the specific language governing permissions and
-- limitations under the License.
--
--
-- CBS Interactive accepts contributions to software products and free
-- and open-source projects owned, licensed, managed, or maintained by
-- CBS Interactive submitted under the terms of the CBS Interactive
-- Contribution License Agreement (the "Contribution Agreement"); you may
-- not submit software to CBS Interactive for inclusion in a CBS
-- Interactive product or project unless you agree to the terms of the
-- CBS Interactive Contribution License Agreement or have executed a
-- separate agreement with CBS Interactive governing the use of such
-- submission. A copy of the Contribution Agreement should have been
-- included with the software. You may also obtain a copy of the
-- Contribution Agreement at
-- http://www.cbsinteractive.com/cbs-interactive-software-grant-and-contribution-license-agreement/.



function dump_apr_sockaddr_t(sa, npre, nsuff, vpre, vsuff)
   local simple_vals =
      {"hostname", "servname", "ipaddr_len", "addr_str_len",
       "port", "next"}

   local puts = rum.core.puts

   for i, name in ipairs(simple_vals) do
      local sval = tostring(sa[name])
      puts(npre .. "." .. name .. nsuff .. vpre.. sval .. vsuff)
   end
end



function dump_apr_table_t(at, npre, nsuff, vpre, vsuff)
   local puts = rum.core.puts
   puts(npre .. nsuff)
   local keys = at:keys()
   for i, v in ipairs(keys) do
      puts("  " .. v .. ":")
      for j, v2 in ipairs(at:raw_mget(v)) do
         puts("    " .. tostring(v2))
      end
   end
end



function dump_conn_rec(c, npre, nsuff, vpre, vsuff)
   local simple_vals =
      {"remote_ip", "remote_host", "remote_logname",
       "local_ip", "local_host", "keepalives",
       "data_in_input_filters", "clogging_input_filters",
       "id", "aborted", "double_reverse"}

   local apr_table_t_vals =
      {"notes"}

   local apr_sockaddr_t_vals =
      {"local_addr", "remote_addr"}

   local puts = rum.core.puts

   for i, name in ipairs(simple_vals) do
      local sval = tostring(c[name])
      puts(npre .. "." .. name .. nsuff .. vpre.. sval .. vsuff)
   end

   for i, name in ipairs(apr_table_t_vals) do
      local at = c[name]
      if at then
         dump_apr_table_t(at, npre .. "." .. name, nsuff, vpre, vsuff)
      else
         puts(npre .. "." .. name .. nsuff .. vpre .. "nil" .. vsuff)
      end
   end

   for i, name in ipairs(apr_sockaddr_t_vals) do
      local sa = c[name]
      if sa then
         dump_apr_sockaddr_t(sa, npre .. "." .. name, nsuff,
                             vpre, vsuff)
      else
         puts(npre .. "." .. name .. nsuff .. vpre .. "nil" .. vsuff)
      end
   end
end



function dump_server_rec(s, npre, nsuff, vpre, vsuff)
   local simple_vals =
      {"defn_name", "server_admin", "server_hostname",
       "error_fname", "path", "server_scheme",
       "defn_line_number", "loglevel", "is_virtual",
       "keep_alive_max", "keep_alive", "pathlen",
       "limit_req_line", "limit_req_fieldsize",
       "limit_req_fields", "port", "timeout",
       "keep_alive_timeout", "next"}

   local puts = rum.core.puts

   for i, name in ipairs(simple_vals) do
      local sval = tostring(s[name])
      puts(npre .. "." .. name .. nsuff .. vpre.. sval .. vsuff)
   end
end



function dump_request_rec(r, npre, nsuff, vpre, vsuff)
   local simple_vals =
      {"the_request", "protocol", "hostname", "status_line",
       "method", "range", "content_type", "handler",
       "content_encoding", "vlist_validator", "user",
       "ap_auth_type", "unparsed_uri", "uri", "filename",
       "canonical_filename", "path_info", "args",
       "assbackwards", "proxyreq", "header_only",
       "proto_num", "status", "method_number", "chunked",
       "read_body", "read_chunked", "expecting_100",
       "no_cache", "no_local_copy", "used_path_info",
       "eos_sent", "next", "prev", "main"}

   local apr_table_t_vals =
      {"headers_in", "headers_out", "err_headers_out",
       "subprocess_env", "notes"}

   local funcs =
      {"is_internal_redirect", "is_subrequest",
       "get_remote_logname", "get_server_name",
       "get_server_port", "default_type", "is_initial_req",
       "http_scheme", "outermost_request"}

   local puts = rum.core.puts

   for i, name in ipairs(funcs) do
      local sval = tostring(r[name](r))
      puts(npre .. ":" .. name .. "()" .. nsuff .. vpre ..
           sval .. vsuff)
   end

   for i, name in ipairs(simple_vals) do
      local sval = tostring(r[name])
      puts(npre .. "." .. name .. nsuff .. vpre .. sval .. vsuff)
   end

   for i, name in ipairs(apr_table_t_vals) do
      local at = r[name]
      if at then
         dump_apr_table_t(at, npre .. "." .. name, nsuff, vpre, vsuff)
      else
         puts(npre .. "." .. name .. nsuff .. vpre .. "nil" .. vsuff)
      end
   end

   if r.connection then
      dump_conn_rec(r.connection, npre .. ".connection", nsuff,
                    vpre, vsuff)
   else
      puts(npre .. ".connection" .. nsuff .. vpre .. "nil" .. vsuff)
   end

   if r.server then
      dump_server_rec(r.server, npre .. ".server" , nsuff,
                      vpre, vsuff)
   else
      puts(npre .. ".server" .. nsuff .. vpre .. "nil" .. vsuff)
   end
end
