/**
 * Copyright 2026 Comcast Cable Communications Management, LLC
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

//#include <strings.h>
//#include <string>
//#include <rbus/rbus.h>
//#include "dm_easy_mesh.h"
//#include <vector>
//#include <utility>
//#include <algorithm>
//#include <cctype>
//#include <functional>
#include "em_ctrl.h"
#include "tr_181.h"

#define MAX_PARAM_LEN 128
/*namespace {
bus_data_prop_t *alloc_string_prop(const char *name, const char *value)
{
    if (name == NULL || value == NULL) {
        return NULL;
    }

    bus_data_prop_t *prop = static_cast<bus_data_prop_t *>(calloc(1, sizeof(*prop)));
    if (prop == NULL) {
        return NULL;
    }

    size_t name_len = strnlen(name, sizeof(prop->name) - 1);
    memcpy(prop->name, name, name_len);
    prop->name[name_len] = '\0';
    prop->name_len = static_cast<uint32_t>(name_len);
    prop->is_data_set = true;
    prop->status = bus_error_success;

    size_t value_len = strlen(value);
    prop->value.data_type = bus_data_type_string;
    prop->value.raw_data.bytes = malloc(value_len + 1);
    if (prop->value.raw_data.bytes == NULL) {
        free(prop);
        return NULL;
    }
    memcpy(prop->value.raw_data.bytes, value, value_len + 1);
    prop->value.raw_data_len = static_cast<unsigned int>(value_len + 1);

    return prop;
}

void set_status_output(raw_data_t *output_data, const char *status)
{
    if (output_data == NULL || status == NULL) {
        return;
    }

    bus_data_prop_t *prop = alloc_string_prop("Status", status);
    if (prop == NULL) {
        return;
    }

    output_data->data_type = bus_data_type_property;
    output_data->raw_data.bytes = prop;
    output_data->raw_data_len = sizeof(bus_data_prop_t);
}
} // namespace
*/
namespace {
bus_data_prop_t *alloc_string_prop(const char *name, const char *value)
{
    if (name == NULL || value == NULL) {
        return NULL;
    }

    bus_data_prop_t *prop = static_cast<bus_data_prop_t *>(calloc(1, sizeof(*prop)));
    if (prop == NULL) {
        return NULL;
    }

    size_t name_len = strnlen(name, sizeof(prop->name) - 1);
    memcpy(prop->name, name, name_len);
    prop->name[name_len] = '\0';
    prop->name_len = static_cast<uint32_t>(name_len);
    prop->is_data_set = true;
    prop->status = bus_error_success;

    size_t value_len = strlen(value);
    prop->value.data_type = bus_data_type_string;
    prop->value.raw_data.bytes = malloc(value_len + 1);
    if (prop->value.raw_data.bytes == NULL) {
        free(prop);
        return NULL;
    }
    memcpy(prop->value.raw_data.bytes, value, value_len + 1);
    prop->value.raw_data_len = static_cast<unsigned int>(value_len + 1);

    return prop;
}


void set_status_output(raw_data_t *output_data, const char *status)
{
    if (output_data == NULL || status == NULL) {
        return;
    }

    bus_data_prop_t *prop = alloc_string_prop("Status", status);
    if (prop == NULL) {
        return;
    }

    output_data->data_type = bus_data_type_property;
    output_data->raw_data.bytes = prop;
    output_data->raw_data_len = sizeof(bus_data_prop_t);
}

bool copy_prop_string(const bus_data_prop_t *prop, char *dst, size_t dst_len)
{
    if (prop == NULL || dst == NULL || dst_len == 0) {
        return false;
    }
    if (prop->value.data_type != bus_data_type_string || prop->value.raw_data.bytes == NULL) {
        return false;
    }

    size_t len = prop->value.raw_data_len;
    if (len == 0) {
        len = strnlen(static_cast<const char *>(prop->value.raw_data.bytes), dst_len - 1);
    }
    if (len >= dst_len) {
        len = dst_len - 1;
    }

    memcpy(dst, prop->value.raw_data.bytes, len);
    dst[len] = '\0';
    return true;
}
} // namespace

//TODO: Rbus abstraction needed for this async method call, it will be enabled once its ready
bus_error_t em_ctrl_t::cmd_setssid(const char *method_name, raw_data_t *input_params, raw_data_t *output_params, void *async_handle)
{
    bus_data_prop_t const *prop = NULL;
    unsigned char buff[EM_IO_BUFF_SZ];
    em_subdoc_info_t *subdoc = NULL;
    cJSON *json = NULL, *root = NULL, *new_json = NULL, *ssid_list = NULL, *target = NULL, *item = NULL, *ssid_item = NULL, *child = NULL, *next = NULL, *band_arr = NULL, *akm_arr = NULL;
    char *updated_json = NULL;
    char ssid[MAX_PARAM_LEN] = {0};
    char addremove[MAX_PARAM_LEN] = {0};
    char passphrase[MAX_PARAM_LEN] = {0};
    char band[MAX_PARAM_LEN] = {0};
    char haul_type[MAX_PARAM_LEN] = {0};

    em_printfout("cmd_setssid invoked for method '%s'", method_name );
    if(input_params == NULL)
    {
        return bus_error_invalid_input;
    }

    em_printfout("cmd_setssid: Processing input parameters");
    prop = reinterpret_cast<const bus_data_prop_t *>(input_params->raw_data.bytes);
    em_printfout("cmd_setssid: Input parameters processed, parsing properties");
    while (prop) {
        em_printfout("cmd_setssid: Processing property '%s'", prop->name);
        if (strcmp(prop->name, "SSID") == 0) {
            em_printfout("cmd_setssid: SSID = '%s'", static_cast<const char *>(prop->value.raw_data.bytes));
            copy_prop_string(prop, ssid, sizeof(ssid));
        } else if (strcmp(prop->name, "AddRemoveChange") == 0) {
            em_printfout("cmd_setssid: AddRemoveChange = '%s'", static_cast<const char *>(prop->value.raw_data.bytes));
            copy_prop_string(prop, addremove, sizeof(addremove));
        } else if (strcmp(prop->name, "Passphrase") == 0) {
            em_printfout("cmd_setssid: Passphrase = '%s'", static_cast<const char *>(prop->value.raw_data.bytes));
            copy_prop_string(prop, passphrase, sizeof(passphrase));
        } else if (strcmp(prop->name, "Band") == 0) {
            em_printfout("cmd_setssid: Band = '%s'", static_cast<const char *>(prop->value.raw_data.bytes));
            copy_prop_string(prop, band, sizeof(band));
        } else if (strcmp(prop->name, "HaulType") ==0) {
            em_printfout("cmd_setssid: HaulType = '%s'", static_cast<const char *>(prop->value.raw_data.bytes));
            copy_prop_string(prop, haul_type, sizeof(haul_type));
        }
        em_printfout("cmd_setssid: Finished processing property '%s'", prop->name);
        prop = prop->next_data;
        em_printfout("cmd_setssid: Moving to next property: %s", prop ? prop->name : "NULL");
    }
    
    if(ssid[0] =='\0' || addremove[0] == '\0')
    {
        em_printfout("ERROR: Missing required parameters in cmd_setssid");
        set_status_output(output_params, "SSID and AddRemoveChange parameters are required");
        return bus_error_invalid_input;
    }

    subdoc = (em_subdoc_info_t *)buff;
    memset(subdoc, 0, sizeof(em_subdoc_info_t));
    strncpy(subdoc->name, "NetworkSSIDList", sizeof(subdoc->name) - 1);
    em_ctrl_t *em_ctrl = em_ctrl_t::get_em_ctrl_instance();
    if (em_ctrl == NULL) {
        em_printfout("ERROR: Controller instance not available");
        set_status_output(output_params, "Failure");
        return bus_error_invalid_input;
    }
    em_ctrl->get_dm_ctrl()->get_config(const_cast<char *>(GLOBAL_NET_ID), subdoc);
    em_printfout("%s:%d: buff=%s \n", __func__, __LINE__, subdoc->buff );
    json = cJSON_Parse(subdoc->buff);
    if (json == NULL) {
        em_printfout("ERROR: Failed to parse JSON from subdoc");
        set_status_output(output_params, "Failure");
        return bus_error_invalid_input;
    }








    set_status_output(output_params, "Success");
    return bus_error_success;

    /*bool use_async = (async_handle != NULL);
    auto respond = [&](const char *status, bus_error_t code) -> bus_error_t {
    const char *resp_status = status ? status : "Failure";
    if (use_async) {
        rbusObject_t resp_obj = NULL;
            rbusObject_Init(&resp_obj, NULL);
            rbusObject_SetPropertyString(resp_obj, "Status", resp_status);
            rbusMethod_SendAsyncResponse(static_cast<rbusMethodAsyncHandle_t>(async_handle),
                                         RBUS_ERROR_SUCCESS, resp_obj);
            rbusObject_Release(resp_obj);
            return bus_error_async_response;
        }
        set_status_output(output_params, resp_status);
        return code;
    };

    if (method_name && method_name[0] != '\0') {
        em_printfout("cmd_setssid invoked for method '%s'", method_name);
    } else {
        em_printfout("cmd_setssid invoked with empty method name");
    }

    ssid_config cfg{};

    em_ctrl_t *em_ctrl = em_ctrl_t::get_em_ctrl_instance();
    em_subdoc_info_t *subdoc = NULL;
    unsigned char buff[EM_IO_BUFF_SZ];
    cJSON *json = NULL, *root = NULL, *new_json = NULL, *ssid_list = NULL, *target = NULL, *item = NULL, *ssid_item = NULL, *child = NULL, *next = NULL, *json_obj = NULL;
    char *jsonbuff = NULL, *updated_json = NULL;
    bus_data_prop_t const *prop = NULL;
    bool enable_set = false;
    bool adv_enabled_set = false;
    bool passphrase_set = false;
    bool mfp_set = false;
    bool suite_set = false;
    bool band_set = false;
    bool akm_set = false;
    bool mobility_set = false;
    bool haul_set = false;
    string_t status_value = "Success";
    int idx = 0;
    size_t json_len = 0;
    std::vector<string_t> band_list;
    std::vector<string_t> akm_list;
    std::vector<string_t> mobility_domain_list;
    std::vector<string_t> haul_type_list;

    if (input_params == NULL) {
        return respond("Error_Invalid_Input", bus_error_invalid_input);
    }

    // Helper functions for safe parsing and normalization of incoming parameters.
    auto dump_bytes = [](const unsigned char *bytes, size_t len, size_t max_out = 32) {
        std::string s;
        size_t n = std::min(len, max_out);
        char buf[4];
        for (size_t i = 0; i < n; ++i) {
            snprintf(buf, sizeof(buf), "%02X", bytes[i]);
            s.append(buf);
            if (i + 1 < n) s.push_back(' ');
        }
        return s;
    };

    // Safely walk the incoming property chain without assuming null-terminated names.
    auto prop_name = [](const bus_data_prop_t *p) -> std::string {
        if (!p) return std::string();
        // Some callers leave name_len unset; fall back to measured length.
        size_t len = p->name_len ? p->name_len : strnlen(p->name, BUS_MAX_NAME_LENGTH - 1);
        len = std::min<size_t>(len, BUS_MAX_NAME_LENGTH - 1);
        return std::string(p->name, p->name + len);
    };

    auto parse_bool_value = [](const bus_data_prop_t *p, bool &value_out) -> bool {
        const unsigned char *raw = reinterpret_cast<const unsigned char *>(p->value.raw_data.bytes);
        size_t len = p->value.raw_data_len;
        if (len == 1 && (raw[0] == 0 || raw[0] == 1)) {
            value_out = (raw[0] != 0);
            return true;
        }
        if (len == 0) {
            return false;
        }
        size_t copy_len = len >= MAX_PARAM_LEN ? MAX_PARAM_LEN - 1 : len;
        char bool_str[MAX_PARAM_LEN] = {0};
        memcpy(bool_str, raw, copy_len);
        bool_str[copy_len] = '\0';
        if (strcasecmp(bool_str, "true") == 0 || strcmp(bool_str, "1") == 0) {
            value_out = true;
            return true;
        }
        if (strcasecmp(bool_str, "false") == 0 || strcmp(bool_str, "0") == 0) {
            value_out = false;
            return true;
        }
        return false;
    };

    auto parse_bool_string = [](const std::string &val, bool &out) -> bool {
        if (val == "1" || strcasecmp(val.c_str(), "true") == 0) { out = true; return true; }
        if (val == "0" || strcasecmp(val.c_str(), "false") == 0) { out = false; return true; }
        return false;
    };

    auto parse_action_string = [](const std::string &val, std::string &out) -> bool {
        static const std::vector<std::string> allowed = {"Add", "Remove", "Change"};
        for (auto const &a : allowed) {
            if (strcasecmp(a.c_str(), val.c_str()) == 0) {
                out = a;
                return true;
            }
        }
        return false;
    };

    auto trim_copy = [](const string_t &val) -> string_t {
        size_t start = 0;
        size_t end = val.size();
        while (start < end && isspace(static_cast<unsigned char>(val[start]))) {
            start++;
        }
        while (end > start && isspace(static_cast<unsigned char>(val[end - 1]))) {
            end--;
        }
        return val.substr(start, end - start);
    };

    auto to_lower_copy = [](string_t val) -> string_t {
        std::transform(val.begin(), val.end(), val.begin(), [](unsigned char c) { return static_cast<char>(tolower(c)); });
        return val;
    };

    auto split_csv = [&](const string_t &src) -> std::vector<string_t> {
        std::vector<string_t> out;
        size_t start = 0;
        while (start < src.size()) {
            size_t comma = src.find(',', start);
            string_t token = (comma == string_t::npos) ? src.substr(start) : src.substr(start, comma - start);
            token = trim_copy(token);
            if (!token.empty()) {
                out.push_back(token);
            }
            if (comma == string_t::npos) {
                break;
            }
            start = comma + 1;
        }
        if (out.empty() && !src.empty()) {
            out.push_back(trim_copy(src));
        }
        return out;
    };

    std::function<bool(const std::string &, const std::string &, const bus_data_prop_t *)> parse_param;

    auto parse_kv_string = [&](const string_t &src) -> std::vector<std::pair<string_t, string_t>> {
        std::vector<std::pair<string_t, string_t>> kvs;
        size_t pos = 0;
        while (pos < src.size()) {
            size_t sep = src.find('=', pos);
            if (sep == string_t::npos) break;
            string_t key = trim_copy(src.substr(pos, sep - pos));
            size_t end = src.find_first_of("\n,;", sep + 1);
            string_t val = trim_copy(src.substr(sep + 1, (end == string_t::npos ? src.size() : end) - (sep + 1)));
            if (!key.empty()) kvs.emplace_back(key, val);
            if (end == string_t::npos) break;
            pos = end + 1;
        }
        return kvs;
    };

    auto canonicalize = [&](const string_t &token, const std::vector<std::pair<const char *, const char *>> &allowed) -> string_t {
        string_t lowered = to_lower_copy(trim_copy(token));
        for (const auto &entry : allowed) {
            if (lowered == entry.first) {
                return entry.second;
            }
        }
        return "";
    };

    auto parse_string_payload = [&](const std::string &payload) -> bool {
        cJSON *root_json = cJSON_Parse(payload.c_str());
        if (root_json && cJSON_IsObject(root_json)) {
            size_t json_field_count = 0;
            for (cJSON *item = root_json->child; item; item = item->next) {
                if (!cJSON_IsString(item) && !cJSON_IsBool(item)) {
                    em_printfout("WARN: Skipping non-string/bool field '%s'", item->string ? item->string : "<null>");
                    continue;
                }
                std::string name = item->string ? item->string : std::string();
                std::string value;
                if (cJSON_IsBool(item)) {
                    value = cJSON_IsTrue(item) ? "true" : "false";
                } else {
                    value = item->valuestring ? item->valuestring : "";
                }
                if (!parse_param(name, value, NULL)) {
                    cJSON_Delete(root_json);
                    return false;
                }
                json_field_count++;
            }
            em_printfout("Processed %zu JSON fields in cmd_setssid", json_field_count);
            cJSON_Delete(root_json);
            return true;
        }

        if (root_json) cJSON_Delete(root_json);
        auto kvs = parse_kv_string(payload);
        if (kvs.empty()) {
            em_printfout("ERROR: Failed to parse string input for cmd_setssid");
            return false;
        }
        for (const auto &kv : kvs) {
            if (!parse_param(kv.first, kv.second, NULL)) {
                return false;
            }
        }
        em_printfout("Processed %zu key/value fields in cmd_setssid", kvs.size());
        return true;
    };

    auto set_string_array = [](cJSON *obj, const char *key, const std::vector<string_t> &values, bool replace) {
        if (values.empty()) {
            return;
        }
        cJSON *arr = cJSON_CreateArray();
        for (const auto &v : values) {
            cJSON_AddItemToArray(arr, cJSON_CreateString(v.c_str()));
        }
        if (replace) {
            cJSON_ReplaceItemInObject(obj, key, arr);
        } else {
            cJSON_AddItemToObject(obj, key, arr);
        }
    };

    auto parse_action_value = [](const bus_data_prop_t *p, string_t &value_out) -> bool {
        em_printfout("Parsing action value from param name='%s', raw value='%.*s'", p->name, (int)p->value.raw_data_len, (char*)p->value.raw_data.bytes);
        const unsigned char *raw = reinterpret_cast<const unsigned char *>(p->value.raw_data.bytes);
        size_t len = p->value.raw_data_len;
        if (len == 0) {
            return false;
        }
        size_t copy_len = len >= MAX_PARAM_LEN ? MAX_PARAM_LEN - 1 : len;
        char action_str[MAX_PARAM_LEN] = {0};
        memcpy(action_str, raw, copy_len);
        action_str[copy_len] = '\0';

        if (strcasecmp(action_str, "add") == 0) {
            value_out = "Add";
            return true;
        }
        if (strcasecmp(action_str, "remove") == 0) {
            value_out = "Remove";
            return true;
        }
        if (strcasecmp(action_str, "change") == 0) {
            value_out = "Change";
            return true;
        }
        return false;
    };

    auto set_string_field = [](cJSON *obj, const char *key, const string_t &val, bool replace) {
        if (val.empty()) {
            return;
        }
        cJSON *node = cJSON_CreateString(val.c_str());
        if (replace) {
            cJSON_ReplaceItemInObject(obj, key, node);
        } else {
            cJSON_AddItemToObject(obj, key, node);
        }
    };

    auto set_bool_field = [](cJSON *obj, const char *key, bool has_value, bool value, bool replace) {
        if (!has_value) {
            return;
        }
        cJSON *node = cJSON_CreateBool(value);
        if (replace) {
            cJSON_ReplaceItemInObject(obj, key, node);
        } else {
            cJSON_AddItemToObject(obj, key, node);
        }
    };

    parse_param = [&](const std::string &name, const std::string &value, const bus_data_prop_t *prop_ctx) -> bool {
        if (name.empty()) {
            status_value = "Error_Invalid_Input";
            em_printfout("ERROR: Empty parameter name in cmd_setssid");
            return false;
        }

        em_printfout("parse_param: name='%s', value='%s'", name.c_str(), value.c_str());

        if (name == "SSID") {
            cfg.ssid = value;
        } else if (name == "PassPhrase") {
            cfg.passphrase = value;
            passphrase_set = true;
        } else if (name == "Band") {
            band_set = true;
            static const std::vector<std::pair<const char *, const char *>> allowed_band = {
                {"all", "All"},
                {"2.4", "2.4"},
                {"5", "5"},
                {"6", "6"},
                {"5_unii_1", "5_UNII_1"},
                {"5_unii_2", "5_UNII_2"},
                {"5_unii_3", "5_UNII_3"},
                {"5_unii_4", "5_UNII_4"},
                {"6_unii_5", "6_UNII_5"},
                {"6_unii_6", "6_UNII_6"},
                {"6_unii_7", "6_UNII_7"},
                {"6_unii_8", "6_UNII_8"},
            };
            auto tokens = split_csv(value);
            if (tokens.empty()) {
                status_value = "Error_Invalid_Input";
                em_printfout("ERROR: Empty Band value in cmd_setssid");
                return false;
            }
            for (const auto &tok : tokens) {
                string_t canonical = canonicalize(tok, allowed_band);
                if (canonical.empty()) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid Band value '%s' in cmd_setssid", tok.c_str());
                    return false;
                }
                band_list.push_back(canonical);
            }
        } else if (name == "AKMsAllowed") {
            akm_set = true;
            static const std::vector<std::pair<const char *, const char *>> allowed_akm = {
                {"psk", "psk"},
                {"dpp", "dpp"},
                {"sae", "sae"},
                {"psk+sae", "psk+sae"},
                {"dpp+sae", "dpp+sae"},
                {"dpp+psk+sae", "dpp+psk+sae"},
                {"suiteselector", "SuiteSelector"},
            };
            auto tokens = split_csv(value);
            if (tokens.empty()) {
                status_value = "Error_Invalid_Input";
                em_printfout("ERROR: Empty AKMsAllowed value in cmd_setssid");
                return false;
            }
            for (const auto &tok : tokens) {
                string_t canonical = canonicalize(tok, allowed_akm);
                if (canonical.empty()) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid AKMsAllowed value '%s' in cmd_setssid", tok.c_str());
                    return false;
                }
                akm_list.push_back(canonical);
            }
        } else if (name == "SuiteSelector") {
            cfg.SuiteSelector = value;
            suite_set = true;
        } else if (name == "AddRemoveChange") {
            if (prop_ctx) {
                if (!parse_action_value(prop_ctx, cfg.AddRemoveChange)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid AddRemoveChange value '%.*s' in cmd_setssid", (int)prop_ctx->value.raw_data_len, (char*)prop_ctx->value.raw_data.bytes);
                    return false;
                }
            } else {
                if (!parse_action_string(value, cfg.AddRemoveChange)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid AddRemoveChange value '%s' in cmd_setssid", value.c_str());
                    return false;
                }
            }
        } else if (name == "Enable") {
            if (prop_ctx) {
                if (!parse_bool_value(prop_ctx, cfg.enable)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid Enable value '%.*s' in cmd_setssid", (int)prop_ctx->value.raw_data_len, (char*)prop_ctx->value.raw_data.bytes);
                    return false;
                }
            } else {
                if (!parse_bool_string(value, cfg.enable)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid Enable value '%s' in cmd_setssid", value.c_str());
                    return false;
                }
            }
            enable_set = true;
        } else if (name == "AdvertisementEnabled") {
            if (prop_ctx) {
                if (!parse_bool_value(prop_ctx, cfg.AdvertisementEnabled)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid AdvertisementEnabled value '%.*s' in cmd_setssid", (int)prop_ctx->value.raw_data_len, (char*)prop_ctx->value.raw_data.bytes);
                    return false;
                }
            } else {
                if (!parse_bool_string(value, cfg.AdvertisementEnabled)) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid AdvertisementEnabled value '%s' in cmd_setssid", value.c_str());
                    return false;
                }
            }
            adv_enabled_set = true;
        } else if (name == "MFPConfig") {
            static const std::vector<std::pair<const char *, const char *>> allowed_mfp = {
                {"disabled", "Disabled"},
                {"optional", "Optional"},
                {"required", "Required"},
            };
            string_t canonical = canonicalize(value, allowed_mfp);
            if (canonical.empty()) {
                status_value = "Error_Invalid_Input";
                em_printfout("ERROR: Invalid MFPConfig value '%s' in cmd_setssid", value.c_str());
                return false;
            }
            cfg.MFPConfig = canonical;
            mfp_set = true;
        } else if (name == "MobilityDomain") {
            mobility_set = true;
            auto tokens = split_csv(value);
            if (tokens.empty()) {
                status_value = "Error_Invalid_Input";
                em_printfout("ERROR: Empty MobilityDomain value in cmd_setssid");
                return false;
            }
            for (const auto &tok : tokens) {
                string_t trimmed = trim_copy(tok);
                if (trimmed.size() != 17) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid MobilityDomain MAC '%s' in cmd_setssid", trimmed.c_str());
                    return false;
                }
                mobility_domain_list.push_back(trimmed);
            }
        } else if (name == "HaulType") {
            haul_set = true;
            static const std::vector<std::pair<const char *, const char *>> allowed_haul = {
                {"fronthaul", "Fronthaul"},
                {"backhaul", "Backhaul"},
            };
            auto tokens = split_csv(value);
            if (tokens.empty()) {
                status_value = "Error_Invalid_Input";
                em_printfout("ERROR: Empty HaulType value in cmd_setssid");
                return false;
            }
            for (const auto &tok : tokens) {
                string_t canonical = canonicalize(tok, allowed_haul);
                if (canonical.empty()) {
                    status_value = "Error_Invalid_Input";
                    em_printfout("ERROR: Invalid HaulType value '%s' in cmd_setssid", tok.c_str());
                    return false;
                }
                haul_type_list.push_back(canonical);
            }
        }

        return true;
    };

    // Unified parsing: try property chain first; if it looks invalid, fall back to JSON/KV string parse.
    if (input_params->raw_data.bytes != NULL) {
        bus_data_prop_t const *input_data = static_cast<bus_data_prop_t const *>(input_params->raw_data.bytes);
        em_printfout("Received parameters in cmd_setssid (raw_len=%u)", input_params->raw_data_len);
        if (input_params->raw_data_len < sizeof(bus_data_prop_t)) {
            em_printfout("WARN: raw payload too small for bus_data_prop_t (len=%u, need>=%zu)", input_params->raw_data_len, sizeof(bus_data_prop_t));
        }

        em_printfout("raw bytes (first %d): %s", 32, dump_bytes(static_cast<const unsigned char*>(input_params->raw_data.bytes), input_params->raw_data_len).c_str());

        std::string first_name = prop_name(input_data);
        bool parsed = false;
        if (!first_name.empty()) {
            em_printfout("Iterating through input parameters (property list):");
            idx = 0;
            const size_t max_params = 64; // guard against corrupt chains
            auto prop = input_data;
            while (prop && idx < static_cast<int>(max_params)) {
                std::string name = prop_name(prop);
                em_printfout("name='%.*s', value='%.*s', len=%u", static_cast<int>(name.size()), name.c_str(), (int)prop->value.raw_data_len, (char*)prop->value.raw_data.bytes, prop->value.raw_data_len);
                std::string value(reinterpret_cast<char *>(prop->value.raw_data.bytes), prop->value.raw_data_len);

                if (!parse_param(name, value, prop)) {
                    return respond(status_value.c_str(), bus_error_invalid_input);
                }

                prop = prop->next_data;
                idx++;
            }
            parsed = true;
        }

        if (!parsed) {
            std::string payload(reinterpret_cast<char *>(input_params->raw_data.bytes), input_params->raw_data_len);
            if (!parse_string_payload(payload)) {
                return respond("Error_Invalid_Input", bus_error_invalid_input);
            }
        }
    } else {
        em_printfout("ERROR: Invalid method parameters format in cmd_setssid (no payload)");
        return respond("Error_Invalid_Input", bus_error_invalid_input);
    }
    em_printfout("Parsing complete. ssid='%s', action='%s', bands=%zu, akms=%zu, mobility_domains=%zu, hauls=%zu", cfg.ssid.c_str(), cfg.AddRemoveChange.c_str(), band_list.size(), akm_list.size(), mobility_domain_list.size(), haul_type_list.size());
    if (cfg.ssid.empty() || cfg.AddRemoveChange.empty()) {
        status_value = "Error_Invalid_Input";
        em_printfout("ERROR: Missing required parameters in cmd_setssid");
        return respond(status_value.c_str(), bus_error_invalid_input);
    }

    bool akm_requests_suite_selector = std::any_of(akm_list.begin(), akm_list.end(), [](const string_t &val) { return val == "SuiteSelector"; });
    if (akm_requests_suite_selector && (!suite_set || cfg.SuiteSelector.empty())) {
        status_value = "Error_Invalid_Input";
        em_printfout("ERROR: SuiteSelector missing while AKMsAllowed requests SuiteSelector in cmd_setssid");
        return respond(status_value.c_str(), bus_error_invalid_input);
    }
    if (suite_set && !akm_requests_suite_selector) {
        status_value = "Error_Invalid_Input";
        em_printfout("ERROR: SuiteSelector provided without AKMsAllowed including SuiteSelector in cmd_setssid");
        return respond(status_value.c_str(), bus_error_invalid_input);
    }

    if (em_ctrl == NULL || em_ctrl->get_dm_ctrl() == NULL) {
        status_value = "Error_Other";
        return respond(status_value.c_str(), bus_error_general);
    }

    subdoc = reinterpret_cast<em_subdoc_info_t *>(buff);
    memset(subdoc, 0, sizeof(*subdoc));
    strncpy(subdoc->name, "NetworkSSIDList", sizeof(subdoc->name) - 1);
    em_ctrl->get_dm_ctrl()->get_config("OneWifiMesh", subdoc);
    em_printfout("buff=%s \n",subdoc->buff );
    json = cJSON_Parse(subdoc->buff);
    if (json == NULL) {
        status_value = "Error_Other";
        em_printfout("ERROR: Failed to parse JSON from subdoc");
        return respond(status_value.c_str(), bus_error_invalid_input);
    }

    root = cJSON_CreateObject();
    // Add "ID" to the beginning of the JSON object
    new_json = cJSON_CreateObject();
    cJSON_AddStringToObject(new_json, "ID", "OneWifiMesh");

    // Move all items from the original json to new_json
    child = json->child;
    while (child) {
        next = child->next;
        cJSON_DetachItemViaPointer(json, child);
        cJSON_AddItemToObject(new_json, child->string, child);
        child = next;
    }
    cJSON_Delete(json);
    json = new_json;

    cJSON_AddItemToObject(root, "wfa-dataelements:SetSSID", json);
    jsonbuff = cJSON_Print(root);
    em_printfout("root: %s\n", jsonbuff);
    free(jsonbuff);

    // Find or add the SSID entry
    ssid_list = cJSON_GetObjectItem(json, "NetworkSSIDList");
    if (ssid_list == NULL || !cJSON_IsArray(ssid_list)) {
        status_value = "Error_Invalid_Input";
        em_printfout("ERROR: NetworkSSIDList not found or is not an array");
        cJSON_Delete(root);
        return respond(status_value.c_str(), bus_error_invalid_input);
    }
    int target_index = -1;
    int array_index = 0;
    cJSON_ArrayForEach(item, ssid_list) {
        ssid_item = cJSON_GetObjectItem(item, "SSID");
        if (ssid_item && !cfg.ssid.empty() && strcmp(ssid_item->valuestring, cfg.ssid.c_str()) == 0) {
            target = item;
            em_printfout("Matching SSID found: %s", ssid_item->valuestring);
            target_index = array_index;
            break;
        }
        //TBD: If not found, update fronthaul for now
        //check if ssid named private_ssdid exists
        ssid_item = cJSON_GetObjectItem(item, "SSID");
        if (ssid_item && !cfg.ssid.empty() && strcmp(ssid_item->valuestring, "private_ssid") == 0) {
            target = item;
            em_printfout("private_ssid found: %s\n", ssid_item->valuestring);
            target_index = array_index;
            break;
        }
        array_index++;
    }
    em_printfout("Target SSID entry: %s", target ? "Found" : "Not Found");
    bool is_add = (strcasecmp(cfg.AddRemoveChange.c_str(), "add") == 0);
    bool is_remove = (strcasecmp(cfg.AddRemoveChange.c_str(), "remove") == 0);
    bool is_change = (strcasecmp(cfg.AddRemoveChange.c_str(), "change") == 0);

    if (!band_set && is_add && !target) {
        band_list.push_back("All");
        band_set = true;
    }

    em_printfout("Action flags: is_add=%d, is_remove=%d, is_change=%d, target_exists=%d", is_add, is_remove, is_change, target ? 1 : 0);

    if ((is_change || (is_add && target)) && target) {
        em_printfout("Replace existing SSID");
        set_string_field(target, "SSID", cfg.ssid, true);
        if (passphrase_set) {
            set_string_field(target, "PassPhrase", cfg.passphrase, true);
        }
        if (band_set) {
            set_string_array(target, "Band", band_list, true);
        }
        if (akm_set) {
            set_string_array(target, "AKMsAllowed", akm_list, true);
        }
        if (suite_set) {
            set_string_field(target, "SuiteSelector", cfg.SuiteSelector, true);
        }
        set_bool_field(target, "Enable", enable_set, cfg.enable, true);
        set_bool_field(target, "AdvertisementEnabled", adv_enabled_set, cfg.AdvertisementEnabled, true);
        if (mfp_set) {
            set_string_field(target, "MFPConfig", cfg.MFPConfig, true);
        }
        if (mobility_set) {
            set_string_array(target, "MobilityDomain", mobility_domain_list, true);
        }
        if (haul_set) {
            set_string_array(target, "HaulType", haul_type_list, true);
        }
        set_string_field(target, "Status", status_value, true);
    } else if (is_remove && target && target_index >= 0) {
        em_printfout("REMOVE existing SSID");
        cJSON_DeleteItemFromArray(ssid_list, target_index);
    } else if (is_add && !target) {
        em_printfout("ADD (new SSID)");
        // Add new entry if SSID does not exist
        target = cJSON_CreateObject();
        cJSON_AddItemToArray(ssid_list, target);
        set_string_field(target, "SSID", cfg.ssid, false);
        if (passphrase_set) {
            set_string_field(target, "PassPhrase", cfg.passphrase, false);
        }
        if (band_set) {
            set_string_array(target, "Band", band_list, false);
        }
        if (akm_set) {
            set_string_array(target, "AKMsAllowed", akm_list, false);
        }
        if (suite_set) {
            set_string_field(target, "SuiteSelector", cfg.SuiteSelector, false);
        }
        set_bool_field(target, "Enable", enable_set, cfg.enable, false);
        set_bool_field(target, "AdvertisementEnabled", adv_enabled_set, cfg.AdvertisementEnabled, false);
        if (mfp_set) {
            set_string_field(target, "MFPConfig", cfg.MFPConfig, false);
        }
        if (mobility_set) {
            set_string_array(target, "MobilityDomain", mobility_domain_list, false);
        }
        if (haul_set) {
            set_string_array(target, "HaulType", haul_type_list, false);
        }
        set_string_field(target, "Status", status_value, false);
    } else {
        em_printfout("ERROR: Invalid AddRemoveChange value or target not found for requested action");
        status_value = "Error_Invalid_Input";
        cJSON_Delete(root);
        return respond(status_value.c_str(), bus_error_invalid_input);
    }

    updated_json = cJSON_PrintUnformatted(root);
    json_len = strlen(updated_json);
    if (json_len >= EM_IO_BUFF_SZ) {
        status_value = "Error_Other";
        em_printfout("ERROR: JSON too large for buffer!");
        free(updated_json);
        cJSON_Delete(root);
        return respond(status_value.c_str(), bus_error_invalid_input);
    }
    
    memcpy(subdoc->buff, updated_json, json_len);
    subdoc->buff[json_len] = '\0';
    json_obj = cJSON_Parse(subdoc->buff);
    if (json_obj) {
        char *new_json = cJSON_Print(json_obj);
        em_printfout("Updated and formatted JSON:\n%s", new_json);
        free(new_json);
        cJSON_Delete(json_obj);
    } else {
        em_printfout("Invalid JSON in subdoc->buff");
    }

    em_printfout("Dispatching IO process for SetSSID; payload length=%zu", strlen(subdoc->buff));
    em_ctrl->io_process(em_bus_event_type_set_ssid, subdoc->buff, strlen(subdoc->buff));
    free(updated_json);
    cJSON_Delete(root);

    return respond(status_value.c_str(), bus_error_success);
*/
}