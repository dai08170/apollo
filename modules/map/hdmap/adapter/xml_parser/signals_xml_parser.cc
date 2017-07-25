/* Copyright 2017 The Apollo Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
=========================================================================*/
#include "modules/map/hdmap/adapter/xml_parser/signals_xml_parser.h"
#include <iomanip>
#include <vector>
#include <string>
#include "modules/map/hdmap/adapter/xml_parser/util_xml_parser.h"
#include "glog/logging.h"

namespace apollo {
namespace hdmap {
namespace adapter {

Status SignalsXmlParser::parse_traffic_lights(const tinyxml2::XMLElement& xml_node,
                            std::vector<TrafficLightInternal>* traffic_lights) {
    CHECK_NOTNULL(traffic_lights);
    const tinyxml2::XMLElement* signal_node = xml_node.FirstChildElement("signal");
    while (signal_node) {
        std::string object_type;
        std::string object_id;
        int checker = UtilXmlParser::query_string_attribute(*signal_node,
                                                        "type", &object_type);
        checker += UtilXmlParser::query_string_attribute(*signal_node,
                                                        "id", &object_id);

        if (checker != tinyxml2::XML_SUCCESS) {
            std::string err_msg = "Error parse signal type.";
            return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
        }

        if (object_type == "trafficLight") {
            PbSignal traffic_light;
            traffic_light.mutable_id()->set_id(object_id);
            std::string layout_type;
            int checker = UtilXmlParser::query_string_attribute(
                                    *signal_node, "layoutType", &layout_type);
            if (checker != tinyxml2::XML_SUCCESS) {
                std::string err_msg = "Error parse signal layout type.";
                return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR,  err_msg);
            }
            PbSignalType signal_layout_type;
            to_pb_signal_type(layout_type, &signal_layout_type);
            traffic_light.set_type(signal_layout_type);

            PbPolygon* polygon = traffic_light.mutable_boundary();
            RETURN_IF_ERROR(UtilXmlParser::parse_outline(*signal_node,
                                                                    polygon));
            const tinyxml2::XMLElement* sub_node =
                                signal_node->FirstChildElement("subsignal");
            while (sub_node) {
                std::string sub_signal_id;
                std::string sub_signal_xml_type;
                checker = UtilXmlParser::query_string_attribute(*sub_node,
                                                "type", &sub_signal_xml_type);
                checker += UtilXmlParser::query_string_attribute(*sub_node,
                                                "id", &sub_signal_id);
                if (checker != tinyxml2::XML_SUCCESS) {
                    std::string err_msg = "Error parse sub signal layout type.";
                    return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
                }

                PbSubSignal* sub_signal = traffic_light.add_subsignal();
                PbSubSignalType sub_signal_type;
                to_pb_subsignal_type(sub_signal_xml_type, &sub_signal_type);
                sub_signal->mutable_id()->set_id(sub_signal_id);
                sub_signal->set_type(sub_signal_type);
                PbPoint3D* pt = sub_signal->mutable_location();
                RETURN_IF_ERROR(UtilXmlParser::parse_point(*sub_node, pt));
                sub_node = sub_node->NextSiblingElement("subsignal");
            }

            TrafficLightInternal trafficlight_internal;
            trafficlight_internal.id = object_id;
            trafficlight_internal.traffic_light = traffic_light;

            sub_node = signal_node->FirstChildElement("stopline");
            if (sub_node) {
                sub_node = sub_node->FirstChildElement("objectReference");
                while (sub_node) {
                    std::string stop_line_id;
                    int checker = UtilXmlParser::query_string_attribute(
                                                *sub_node, "id", &stop_line_id);
                    assert(checker == tinyxml2::XML_SUCCESS);
                    trafficlight_internal.stop_line_ids.insert(stop_line_id);
                    sub_node = sub_node->NextSiblingElement("objectReference");
                }
            }

            traffic_lights->emplace_back(trafficlight_internal);
        }

        signal_node = signal_node->NextSiblingElement("signal");
    }
    return Status::OK();
}

Status SignalsXmlParser::to_pb_signal_type(const std::string& xml_type,
                            PbSignalType* signal_type) {
    CHECK_NOTNULL(signal_type);

    std::string upper_str(xml_type);
    UtilXmlParser::to_upper(&upper_str);

    if (upper_str == "UNKNOWN") {
        *signal_type = ::apollo::hdmap::Signal::UNKNOWN;
    } else if (upper_str == "MIX2HORIZONTAL") {
        *signal_type = ::apollo::hdmap::Signal::MIX_2_HORIZONTAL;
    } else if (upper_str == "MIX2VERTICAL") {
        *signal_type = ::apollo::hdmap::Signal::MIX_2_VERTICAL;
    } else if (upper_str == "MIX3HORIZONTAL") {
        *signal_type = ::apollo::hdmap::Signal::MIX_3_HORIZONTAL;
    } else if (upper_str == "MIX3VERTICAL") {
        *signal_type = ::apollo::hdmap::Signal::MIX_3_VERTICAL;
    } else if (upper_str == "SINGLE") {
        *signal_type = ::apollo::hdmap::Signal::SINGLE;
    } else {
        std::string err_msg = "Error or unsupport signal layout type";
        return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
    }

    return Status::OK();
}

Status SignalsXmlParser::to_pb_subsignal_type(const std::string& xml_type,
                            PbSubSignalType* sub_signal_type) {
    CHECK_NOTNULL(sub_signal_type);

    std::string upper_str(xml_type);
    UtilXmlParser::to_upper(&upper_str);

    if (upper_str == "UNKNOWN") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::UNKNOWN;
    } else if (upper_str == "CIRCLE") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::CIRCLE;
    } else if (upper_str == "ARROWLEFT") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_LEFT;
    } else if (upper_str == "ARROWFORWARD") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_FORWARD;
    } else if (upper_str == "ARROWRIGHT") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_RIGHT;
    } else if (upper_str == "ARROWLEFTANDFORWARD") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_LEFT_AND_FORWARD;
    } else if (upper_str == "ARROWRIGHTANDFORWARD") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_RIGHT_AND_FORWARD;
    } else if (upper_str == "ARROWUTURN") {
        *sub_signal_type = ::apollo::hdmap::Subsignal::ARROW_U_TURN;
    } else {
        std::string err_msg = "Error or unsupport sub signal type";
        return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
    }
    return Status::OK();
}

Status SignalsXmlParser::parse_stop_signs(const tinyxml2::XMLElement& xml_node,
                                std::vector<StopSignInternal>* stop_signs) {
    CHECK_NOTNULL(stop_signs);

    const tinyxml2::XMLElement* signal_node = xml_node.FirstChildElement("signal");
    while (signal_node) {
        std::string object_type;
        std::string object_id;
        int checker = UtilXmlParser::query_string_attribute(*signal_node,
                                                        "type", &object_type);
        checker += UtilXmlParser::query_string_attribute(*signal_node,
                                                        "id", &object_id);
        if (checker != tinyxml2::XML_SUCCESS) {
            std::string err_msg = "Error parse signal type.";
            return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
        }

        if (object_type == "stopSign") {
            PbStopSign stop_sign;
            stop_sign.mutable_id()->set_id(object_id);

            StopSignInternal stop_sign_internal;
            stop_sign_internal.stop_sign = stop_sign;

            const tinyxml2::XMLElement* sub_node =
                                    signal_node->FirstChildElement("stopline");
            if (sub_node) {
                sub_node = sub_node->FirstChildElement("objectReference");
                while (sub_node) {
                    std::string stop_line_id;
                    int checker = UtilXmlParser::query_string_attribute(
                                                *sub_node, "id", &stop_line_id);
                    assert(checker != tinyxml2::XML_SUCCESS);
                    stop_sign_internal.stop_line_ids.insert(stop_line_id);

                    sub_node = sub_node->NextSiblingElement("objectReference");
                }
            }
            stop_signs->emplace_back(stop_sign_internal);
        }

        signal_node = signal_node->NextSiblingElement("signal");
    }
    return Status::OK();
}

Status SignalsXmlParser::parse_yield_signs(const tinyxml2::XMLElement& xml_node,
                                std::vector<YieldSignInternal>* yield_signs) {
    CHECK_NOTNULL(yield_signs);

    const tinyxml2::XMLElement* signal_node = xml_node.FirstChildElement("signal");
    while (signal_node) {
        std::string object_type;
        std::string object_id;
        int checker = UtilXmlParser::query_string_attribute(*signal_node,
                                                        "type", &object_type);
        checker += UtilXmlParser::query_string_attribute(*signal_node,
                                                        "id", &object_id);
        if (checker != tinyxml2::XML_SUCCESS) {
            std::string err_msg = "Error parse signal type.";
            return Status(apollo::common::ErrorCode::HDMAP_DATA_ERROR, err_msg);
        }

        if (object_type == "yieldSign") {
            PbYieldSign yield_sign;
            yield_sign.mutable_id()->set_id(object_id);
            YieldSignInternal yield_sign_internal;
            yield_sign_internal.id = object_id;
            yield_sign_internal.yield_sign = yield_sign;
            const tinyxml2::XMLElement* sub_node
                                = signal_node->FirstChildElement("stopline");
            if (sub_node) {
                sub_node = sub_node->FirstChildElement("objectReference");
                while (sub_node) {
                    std::string stop_line_id;
                    int checker = UtilXmlParser::query_string_attribute(
                                                *sub_node, "id", &stop_line_id);
                    assert(checker == tinyxml2::XML_SUCCESS);
                    yield_sign_internal.stop_line_ids.insert(stop_line_id);

                    sub_node = sub_node->NextSiblingElement("objectReference");
                }
            }
            yield_signs->emplace_back(yield_sign_internal);
        }

        signal_node = signal_node->NextSiblingElement("signal");
    }
    return Status::OK();
}

}  // namespace adapter
}  // namespace hdmap
}  // namespace apollo
