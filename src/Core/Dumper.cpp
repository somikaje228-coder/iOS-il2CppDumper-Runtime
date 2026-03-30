#include "Dumper.hpp"

#pragma GCC diagnostic ignored "-Wundefined-internal"

namespace Dumper {
    void *domain = nullptr;
    DumpStatus status = DumpStatus::NONE;
    std::string dumpDir = "";
    namespace GenScript {
        json jsonData = json::object();
        File scriptFile = File();
        std::unordered_set<uint64_t> dataOffsets = std::unordered_set<uint64_t>();
        std::unordered_map<uint64_t, uint64_t> typeInfoAddrs;
    }
}

void Dumper::init() {
    Dumper::domain = Variables::IL2CPP::il2cpp_domain_get();
#if GENSCRIPT
    GenScript::init();
#endif
}

std::vector<void *> Dumper::getAssemblies() {
    size_t size;
    void **assemblies = Variables::IL2CPP::il2cpp_domain_get_assemblies(Dumper::domain, &size);
    return std::vector<void *>(assemblies, assemblies + size);
}

std::vector<void *> Dumper::getClasses(void *image) {
    std::vector<void *> classes;
    const Variables::Il2CppImage *il2cppImage = static_cast<Variables::Il2CppImage *>(image);
    size_t classCount = Variables::IL2CPP::il2cpp_image_get_class_count(il2cppImage);

    for (size_t i = 0; i < classCount; ++i) {
        classes.push_back(Variables::IL2CPP::il2cpp_image_get_class(il2cppImage, i));
    }
    return classes;
}

Dumper::DumpStatus Dumper::dump(const std::string &dir, const std::string &headers_dir) {
    std::stringstream dumpOut;
    std::stringstream log;
    dumpDir = dir;

    Dumper::init();

    File dumpFile(dir + "/dump.cs", "w");
    if (!dumpFile.ok()) return Dumper::DumpStatus::ERROR;

    if (headers_dir.empty()) return Dumper::DumpStatus::ERROR;

    Log("BaseAddress: 0x%llx", Variables::info.address);
    Log("==========================");
    Log("Init Dumping...");

    auto assemblies = Dumper::getAssemblies();
    Log("Total Assemblies: %d", assemblies.size());

    for (int i = 0; i < assemblies.size(); i++) {
        const void *image = Variables::IL2CPP::il2cpp_assembly_get_image(assemblies[i]);
        dumpOut << "// Image " << i << ": " << Variables::IL2CPP::il2cpp_image_get_name((void *)image) << std::endl;
    }

    for (auto assembly : assemblies) {
        const void *image = Variables::IL2CPP::il2cpp_assembly_get_image(assembly);
        const char *imageName = Variables::IL2CPP::il2cpp_image_get_name((void *)image);

        std::string assemblyFileName = headers_dir + "/" + (imageName ? imageName : "unknown") + ".cs";
        std::stringstream singleAssemblyOutPut;
        auto classes = Dumper::getClasses((void *)image);
        Log("Total Classes in %s: %d", imageName ? imageName : "unknown", classes.size());

        for (auto klass : classes) {
            const char *classNamespace = Variables::IL2CPP::il2cpp_class_get_namespace(klass);
            std::string nsStr = (classNamespace ? std::string(classNamespace) : "");
            dumpOut << "// Namespace: " << nsStr << std::endl;
            singleAssemblyOutPut << "// Namespace: " << nsStr << std::endl;

            auto flags = Variables::IL2CPP::il2cpp_class_get_flags(klass);
            auto isEnum = Variables::IL2CPP::il2cpp_class_is_enum(klass);
            auto isValueType = Variables::IL2CPP::il2cpp_class_is_valuetype(klass);
            auto visibility = flags & TYPE_ATTRIBUTE_VISIBILITY_MASK;
            switch (visibility) {
                case TYPE_ATTRIBUTE_PUBLIC:
                case TYPE_ATTRIBUTE_NESTED_PUBLIC:
                    dumpOut << "public ";
                    singleAssemblyOutPut << "public ";
                    break;
                case TYPE_ATTRIBUTE_NOT_PUBLIC:
                case TYPE_ATTRIBUTE_NESTED_FAM_AND_ASSEM:
                case TYPE_ATTRIBUTE_NESTED_ASSEMBLY:
                    dumpOut << "internal ";
                    singleAssemblyOutPut << "internal ";
                    break;
                case TYPE_ATTRIBUTE_NESTED_PRIVATE:
                    dumpOut << "private ";
                    singleAssemblyOutPut << "private ";
                    break;
                case TYPE_ATTRIBUTE_NESTED_FAMILY:
                    dumpOut << "protected ";
                    singleAssemblyOutPut << "protected ";
                    break;
                case TYPE_ATTRIBUTE_NESTED_FAM_OR_ASSEM:
                    dumpOut << "protected internal ";
                    singleAssemblyOutPut << "protected internal ";
                    break;
            }

            if (flags & TYPE_ATTRIBUTE_ABSTRACT && flags & TYPE_ATTRIBUTE_SEALED) {
                dumpOut << "static ";
                singleAssemblyOutPut << "static ";
            } else if (!(flags & TYPE_ATTRIBUTE_INTERFACE) && flags & TYPE_ATTRIBUTE_ABSTRACT) {
                dumpOut << "abstract ";
                singleAssemblyOutPut << "abstract ";
            } else if (!isValueType && !isEnum && flags & TYPE_ATTRIBUTE_SEALED) {
                dumpOut << "sealed ";
                singleAssemblyOutPut << "sealed ";
            }
            if (flags & TYPE_ATTRIBUTE_INTERFACE) {
                dumpOut << "interface ";
                singleAssemblyOutPut << "interface ";
            } else if (isEnum) {
                dumpOut << "enum ";
                singleAssemblyOutPut << "enum ";
            } else if (isValueType) {
                dumpOut << "struct ";
                singleAssemblyOutPut << "struct ";
            } else {
                dumpOut << "class ";
                singleAssemblyOutPut << "class ";
            }

            std::string className = getClassName(klass);
            dumpOut << className;
            singleAssemblyOutPut << className;

            // Base types
            std::vector<std::string> baseTypes;
            auto parent = Variables::IL2CPP::il2cpp_class_get_parent(klass);
            if (parent) {
                std::string pName = getClassName(parent);
                if (pName != "Object" && pName != "ValueType" && pName != "Enum") {
                    void* pType = Variables::IL2CPP::il2cpp_class_get_type(parent);
                    baseTypes.push_back(pType ? getTypeName(pType) : pName);
                }
            }

            void *iIter = nullptr;
            while (auto iface = Variables::IL2CPP::il2cpp_class_get_interfaces(klass, &iIter)) {
                void* iType = Variables::IL2CPP::il2cpp_class_get_type(iface);
                baseTypes.push_back(iType ? getTypeName(iType) : getClassName(iface));
            }

            if (!baseTypes.empty()) {
                dumpOut << " : ";
                singleAssemblyOutPut << " : ";
                for (size_t b = 0; b < baseTypes.size(); b++) {
                    if (b > 0) {
                        dumpOut << ", ";
                        singleAssemblyOutPut << ", ";
                    }
                    dumpOut << baseTypes[b];
                    singleAssemblyOutPut << baseTypes[b];
                }
            }

            dumpOut << std::endl;
            singleAssemblyOutPut << std::endl;
            dumpOut << "{";
            singleAssemblyOutPut << "{";

            dumpOut << dumpField(klass);
            singleAssemblyOutPut << dumpField(klass);

            dumpOut << dumpProperty(klass);
            singleAssemblyOutPut << dumpProperty(klass);

            dumpOut << dumpMethod(klass);
            singleAssemblyOutPut << dumpMethod(klass);

            dumpOut << "}\n\n";
            singleAssemblyOutPut << "}\n\n";
        }
        dumpOut << std::endl;
        singleAssemblyOutPut << std::endl;

        File singleAssemblyFile(assemblyFileName, "w");
        if (singleAssemblyFile.ok()) {
            singleAssemblyFile.write(singleAssemblyOutPut);
            singleAssemblyFile.close();
        }
    }
    Log("Dumping Completed.");
    dumpFile.write(dumpOut);
    dumpFile.close();

    // Generate il2cpp.h
    Log("Generating il2cpp.h...");
    std::string il2cpphPath = dumpDir + "/il2cpp.h";
    File hFile(il2cpphPath, "w");
    if (hFile.ok()) {
        hFile.write(
            "typedef void(*Il2CppMethodPointer)();\n"
            "struct MethodInfo;\n\n"
            "struct VirtualInvokeData {\n"
            "\tIl2CppMethodPointer methodPtr;\n"
            "\tconst MethodInfo* method;\n"
            "};\n\n"
            "struct Il2CppType {\n"
            "\tvoid* data;\n"
            "\tunsigned int bits;\n"
            "};\n\n"
            "struct Il2CppClass;\n\n"
            "struct Il2CppObject {\n"
            "\tIl2CppClass *klass;\n"
            "\tvoid *monitor;\n"
            "};\n\n"
            "union Il2CppRGCTXData {\n"
            "\tvoid* rgctxDataDummy;\n"
            "\tconst MethodInfo* method;\n"
            "\tconst Il2CppType* type;\n"
            "\tIl2CppClass* klass;\n"
            "};\n\n"
            "struct Il2CppRuntimeInterfaceOffsetPair {\n"
            "\tIl2CppClass* interfaceType;\n"
            "\tint32_t offset;\n"
            "};\n\n"
            "struct Il2CppClass_1 {\n"
            "\tvoid* image;\n"
            "\tvoid* gc_desc;\n"
            "\tconst char* name;\n"
            "\tconst char* namespaze;\n"
            "\tIl2CppType byval_arg;\n"
            "\tIl2CppType this_arg;\n"
            "\tIl2CppClass* element_class;\n"
            "\tIl2CppClass* castClass;\n"
            "\tIl2CppClass* declaringType;\n"
            "\tIl2CppClass* parent;\n"
            "\tvoid *generic_class;\n"
            "\tvoid* typeDefinition;\n"
            "\tvoid* interopData;\n"
            "\tIl2CppClass* klass;\n"
            "\tvoid* fields;\n"
            "\tvoid* events;\n"
            "\tvoid* properties;\n"
            "\tvoid* methods;\n"
            "\tIl2CppClass** nestedTypes;\n"
            "\tIl2CppClass** implementedInterfaces;\n"
            "\tIl2CppRuntimeInterfaceOffsetPair* interfaceOffsets;\n"
            "};\n\n"
            "struct Il2CppClass_2 {\n"
            "\tIl2CppClass** typeHierarchy;\n"
            "\tvoid *unity_user_data;\n"
            "\tuint32_t initializationExceptionGCHandle;\n"
            "\tuint32_t cctor_started;\n"
            "\tuint32_t cctor_finished;\n"
            "\tsize_t cctor_thread;\n"
            "\tint32_t genericContainerIndex;\n"
            "\tuint32_t instance_size;\n"
            "\tuint32_t actualSize;\n"
            "\tuint32_t element_size;\n"
            "\tint32_t native_size;\n"
            "\tuint32_t static_fields_size;\n"
            "\tuint32_t thread_static_fields_size;\n"
            "\tint32_t thread_static_fields_offset;\n"
            "\tuint32_t flags;\n"
            "\tuint32_t token;\n"
            "\tuint16_t method_count;\n"
            "\tuint16_t property_count;\n"
            "\tuint16_t field_count;\n"
            "\tuint16_t event_count;\n"
            "\tuint16_t nested_type_count;\n"
            "\tuint16_t vtable_count;\n"
            "\tuint16_t interfaces_count;\n"
            "\tuint16_t interface_offsets_count;\n"
            "\tuint8_t typeHierarchyDepth;\n"
            "\tuint8_t genericRecursionDepth;\n"
            "\tuint8_t rank;\n"
            "\tuint8_t minimumAlignment;\n"
            "\tuint8_t naturalAligment;\n"
            "\tuint8_t packingSize;\n"
            "\tuint8_t bitflags1;\n"
            "\tuint8_t bitflags2;\n"
            "};\n\n"
            "struct Il2CppClass {\n"
            "\tIl2CppClass_1 _1;\n"
            "\tvoid* static_fields;\n"
            "\tIl2CppRGCTXData* rgctx_data;\n"
            "\tIl2CppClass_2 _2;\n"
            "\tVirtualInvokeData vtable[255];\n"
            "};\n\n"
            "struct MethodInfo {\n"
            "\tIl2CppMethodPointer methodPointer;\n"
            "\tvoid* invoker_method;\n"
            "\tconst char* name;\n"
            "\tIl2CppClass *klass;\n"
            "\tconst Il2CppType *return_type;\n"
            "\tconst void* parameters;\n"
            "\tuint32_t token;\n"
            "\tuint16_t flags;\n"
            "\tuint16_t iflags;\n"
            "\tuint16_t slot;\n"
            "\tuint8_t parameters_count;\n"
            "\tuint8_t bitflags;\n"
            "};\n"
        );

        std::vector<void*> allClasses;
        for (auto assembly : assemblies) {
            const void *image = Variables::IL2CPP::il2cpp_assembly_get_image(assembly);
            auto classes = Dumper::getClasses((void *)image);
            allClasses.insert(allClasses.end(), classes.begin(), classes.end());
        }

        for (auto klass : allClasses) {
            std::string s = dumpCStruct(klass);
            if (!s.empty()) hFile.write(s.c_str());
        }

        hFile.close();
        Log("il2cpp.h Generated.");
    }

#if GENSCRIPT
    GenScript::save();
    GenScript::scriptFile.close();
#endif
    return Dumper::DumpStatus::SUCCESS;
}

std::string Dumper::dumpField(void *klass) {
    std::stringstream outPut;
    outPut << "\n\t// Fields\n";
    auto is_enum = Variables::IL2CPP::il2cpp_class_is_enum(klass);
    void *iter = nullptr;
    while (auto field = Variables::IL2CPP::il2cpp_class_get_fields(klass, &iter)) {
        outPut << "\t";
        auto attrs = Variables::IL2CPP::il2cpp_field_get_flags(field);
        auto access = attrs & FIELD_ATTRIBUTE_FIELD_ACCESS_MASK;
        switch (access) {
            case FIELD_ATTRIBUTE_PRIVATE:
                outPut << "private ";
                break;
            case FIELD_ATTRIBUTE_PUBLIC:
                outPut << "public ";
                break;
            case FIELD_ATTRIBUTE_FAMILY:
                outPut << "protected ";
                break;
            case FIELD_ATTRIBUTE_ASSEMBLY:
            case FIELD_ATTRIBUTE_FAM_AND_ASSEM:
                outPut << "internal ";
                break;
            case FIELD_ATTRIBUTE_FAM_OR_ASSEM:
                outPut << "protected internal ";
                break;
        }
        if (attrs & FIELD_ATTRIBUTE_LITERAL) {
            outPut << "const ";
        } else {
            if (attrs & FIELD_ATTRIBUTE_STATIC) {
                outPut << "static ";
            }
            if (attrs & FIELD_ATTRIBUTE_INIT_ONLY) {
                outPut << "readonly ";
            }
        }

        auto field_type = Variables::IL2CPP::il2cpp_field_get_type(field);
        auto field_class = field_type ? Variables::IL2CPP::il2cpp_class_from_type(field_type) : nullptr;
        std::string field_type_name = field_type ? getTypeName(field_type) : "unknown";
        std::string field_class_name = field_class ? getClassName(field_class) : "unknown";
        const char *field_name = Variables::IL2CPP::il2cpp_field_get_name(field);
        outPut << field_type_name << " " << (field_name ? field_name : "unknown_field");

        if (attrs & FIELD_ATTRIBUTE_LITERAL && is_enum) {
            uint64_t val = 0;
            Variables::IL2CPP::il2cpp_field_static_get_value(field, &val);
            outPut << " = " << std::dec << val << ";\n";
        } else if (attrs & FIELD_ATTRIBUTE_LITERAL) {
            if (field_type_name == "String" || field_type_name == "string" || field_class_name == "String") {
                void *val = nullptr;
                Variables::IL2CPP::il2cpp_field_static_get_value(field, &val);
                if (!val) {
                    outPut << " = null;\n";
                    continue;
                }
                uint16_t *chars = Variables::IL2CPP::il2cpp_string_chars(val);
                std::string strValue = uint16ToString(chars);
                outPut << " = \"" << strValue << "\";\n";
#if GENSCRIPT
                GenScript::addString((uint64_t)val - Variables::info.address, strValue);
#endif
            } else {
                uint64_t val = 0;
                Variables::IL2CPP::il2cpp_field_static_get_value(field, &val);
                outPut << " = " << std::dec << val << ";\n";
            }
        } else {
            outPut << "; // 0x" << std::hex << std::uppercase << Variables::IL2CPP::il2cpp_field_get_offset(field) << "\n";
        }
    }
    if (outPut.str().length() == 12) return "";
    return outPut.str();
}

std::string Dumper::dumpProperty(void *klass) {
    std::stringstream outPut;
    outPut << "\n\t// Properties\n";
    void *iter = nullptr;
    while (auto prop = Variables::IL2CPP::il2cpp_class_get_properties(klass, &iter)) {
        auto get = Variables::IL2CPP::il2cpp_property_get_get_method(prop);
        auto set = Variables::IL2CPP::il2cpp_property_get_set_method(prop);
        auto prop_name = Variables::IL2CPP::il2cpp_property_get_name(prop);
        outPut << "\t";
        void *prop_class = nullptr;
        uint32_t iflags = 0;
        if (get) {
            outPut << getMethodModifier(Variables::IL2CPP::il2cpp_method_get_flags(get, &iflags));
            auto retType = Variables::IL2CPP::il2cpp_method_get_return_type(get);
            prop_class = retType ? Variables::IL2CPP::il2cpp_class_from_type(retType) : nullptr;
            if (prop_class) {
                outPut << getTypeName(retType) << " " << (prop_name ? prop_name : "unknown_property") << " { ";
                if (get) outPut << "get; ";
                if (set) outPut << "set; ";
                outPut << "}\n";
            } else {
                if (prop_name) outPut << " // unknown property " << prop_name << "\n";
                else outPut << " // unknown property\n";
            }
        } else if (set) {
            outPut << getMethodModifier(Variables::IL2CPP::il2cpp_method_get_flags(set, &iflags));
            auto param = Variables::IL2CPP::il2cpp_method_get_param(set, 0);
            prop_class = param ? Variables::IL2CPP::il2cpp_class_from_type(param) : nullptr;
            if (prop_class) {
                outPut << getTypeName(param) << " " << (prop_name ? prop_name : "unknown_property") << " { set; }\n";
            } else {
                if (prop_name) outPut << " // unknown property " << prop_name << "\n";
                else outPut << " // unknown property\n";
            }
        }
    }
    if (outPut.str().length() == 16) return "";
    return outPut.str();
}

std::string Dumper::dumpMethod(void *klass) {
    std::stringstream outPut;
    outPut << "\n\t// Methods\n\n";
    void *iter = nullptr;
    while (auto method = Variables::IL2CPP::il2cpp_class_get_methods(klass, &iter)) {
        uint32_t iflags = 0;
        auto flags = Variables::IL2CPP::il2cpp_method_get_flags(method, &iflags);

        auto methodPointer = *(void **)method;
        if (!methodPointer || flags & METHOD_ATTRIBUTE_ABSTRACT) {
            outPut << "\t// RVA: -1 Offset: -1 VA: -1";
        } else {
            uint64_t rva = (uint64_t)methodPointer - Variables::info.address;
            uint64_t va = rva;
            if (Variables::info.index == 0) {
                va += 0x100000000;
            }
            outPut << "\t// RVA: 0x" << std::hex << std::uppercase << rva
                   << " Offset: 0x" << rva
                   << " VA: 0x" << va;
        }
        outPut << "\n\t";
        outPut << getMethodModifier(flags);

        auto return_type = Variables::IL2CPP::il2cpp_method_get_return_type(method);
        if (return_type && Variables::IL2CPP::il2cpp_type_is_byref(return_type)) {
            outPut << "ref ";
        }
        outPut << (return_type ? getTypeName(return_type) : "void") << " ";

        const char *methodName = Variables::IL2CPP::il2cpp_method_get_name(method);
        outPut << (methodName ? methodName : "unknown_method") << "(";

        auto param_count = Variables::IL2CPP::il2cpp_method_get_param_count(method);
        for (int i = 0; i < param_count; ++i) {
            auto param = Variables::IL2CPP::il2cpp_method_get_param(method, i);
            if (!param) { outPut << "unknown, "; continue; }
            auto attrs = Variables::IL2CPP::il2cpp_type_get_attrs(param);
            if (Variables::IL2CPP::il2cpp_type_is_byref(param)) {
                if (attrs & PARAM_ATTRIBUTE_OUT && !(attrs & PARAM_ATTRIBUTE_IN)) {
                    outPut << "out ";
                } else if (attrs & PARAM_ATTRIBUTE_IN && !(attrs & PARAM_ATTRIBUTE_OUT)) {
                    outPut << "in ";
                } else {
                    outPut << "ref ";
                }
            } else {
                if (attrs & PARAM_ATTRIBUTE_IN) outPut << "[In] ";
                if (attrs & PARAM_ATTRIBUTE_OUT) outPut << "[Out] ";
            }
            const char *paramName = Variables::IL2CPP::il2cpp_method_get_param_name(method, i);
            outPut << getTypeName(param) << " " << (paramName ? paramName : "");
            outPut << ", ";
        }
        if (param_count > 0) {
            outPut.seekp(-2, outPut.cur);
        }
        outPut << ") { }\n\n";

#if GENSCRIPT
        {
            const char *classNamespace = Variables::IL2CPP::il2cpp_class_get_namespace(klass);
            std::string cls = getClassName(klass);
            uint64_t methodAddr = methodPointer ? (uint64_t)methodPointer - Variables::info.address : 0;
            if (methodPointer && (flags & METHOD_ATTRIBUTE_ABSTRACT) == 0) {
                GenScript::addMethod(methodAddr,
                    classNamespace ? std::string(classNamespace) : "",
                    cls,
                    methodName ? std::string(methodName) : "",
                    method);
            }
            GenScript::addMetadataMethod(method, methodAddr,
                classNamespace ? std::string(classNamespace) : "",
                cls,
                methodName ? std::string(methodName) : "");
        }
#endif
    }
    if (outPut.str().length() == 14) return "";
    return outPut.str();
}

std::string Dumper::getMethodModifier(uint32_t flags) {
    std::stringstream outPut;
    auto access = flags & METHOD_ATTRIBUTE_MEMBER_ACCESS_MASK;
    switch (access) {
        case METHOD_ATTRIBUTE_PRIVATE:   outPut << "private ";           break;
        case METHOD_ATTRIBUTE_PUBLIC:    outPut << "public ";            break;
        case METHOD_ATTRIBUTE_FAMILY:    outPut << "protected ";         break;
        case METHOD_ATTRIBUTE_ASSEM:
        case METHOD_ATTRIBUTE_FAM_AND_ASSEM: outPut << "internal ";     break;
        case METHOD_ATTRIBUTE_FAM_OR_ASSEM: outPut << "protected internal "; break;
    }
    if (flags & METHOD_ATTRIBUTE_STATIC)  outPut << "static ";
    if (flags & METHOD_ATTRIBUTE_ABSTRACT) {
        outPut << "abstract ";
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            outPut << "override ";
    } else if (flags & METHOD_ATTRIBUTE_FINAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_REUSE_SLOT)
            outPut << "sealed override ";
    } else if (flags & METHOD_ATTRIBUTE_VIRTUAL) {
        if ((flags & METHOD_ATTRIBUTE_VTABLE_LAYOUT_MASK) == METHOD_ATTRIBUTE_NEW_SLOT)
            outPut << "virtual ";
        else
            outPut << "override ";
    }
    if (flags & METHOD_ATTRIBUTE_PINVOKE_IMPL) outPut << "extern ";
    return outPut.str();
}

std::string Dumper::getClassName(void *klass) {
    if (!klass) return "unknown";
    const char *klassName = Variables::IL2CPP::il2cpp_class_get_name(klass);
    if (!klassName) return "unknown";
    std::string name(klassName);
    auto backtick = name.find('`');
    if (backtick != std::string::npos) {
        std::string baseName = name.substr(0, backtick);
        int numArgs = 0;
        if (backtick + 1 < name.size()) numArgs = name[backtick + 1] - '0';
        std::stringstream outPut;
        outPut << baseName << "<";
        for (int i = 0; i < numArgs; i++) {
            if (i > 0) outPut << ", ";
            outPut << (numArgs == 1 ? "T" : "T" + std::to_string(i + 1));
        }
        outPut << ">";
        return outPut.str();
    }
    return name;
}

std::string Dumper::mapTypeName(const std::string& name) {
    static const std::unordered_map<std::string, std::string> typeMap = {
        {"Void", "void"}, {"Boolean", "bool"}, {"Byte", "byte"}, {"SByte", "sbyte"},
        {"Int16", "short"}, {"UInt16", "ushort"}, {"Int32", "int"}, {"UInt32", "uint"},
        {"Int64", "long"}, {"UInt64", "ulong"}, {"Single", "float"}, {"Double", "double"},
        {"String", "string"}, {"Object", "object"}, {"Char", "char"},
        {"IntPtr", "IntPtr"}, {"UIntPtr", "UIntPtr"},
        {"System.Void", "void"}, {"System.Boolean", "bool"}, {"System.Byte", "byte"},
        {"System.SByte", "sbyte"}, {"System.Int16", "short"}, {"System.UInt16", "ushort"},
        {"System.Int32", "int"}, {"System.UInt32", "uint"}, {"System.Int64", "long"},
        {"System.UInt64", "ulong"}, {"System.Single", "float"}, {"System.Double", "double"},
        {"System.String", "string"}, {"System.Object", "object"}, {"System.Char", "char"},
        {"System.IntPtr", "IntPtr"}, {"System.UIntPtr", "UIntPtr"},
    };

    auto it = typeMap.find(name);
    if (it != typeMap.end()) return it->second;

    std::string result;
    std::string token;
    for (size_t i = 0; i <= name.size(); i++) {
        char c = (i < name.size()) ? name[i] : '\0';
        if (c == '<' || c == '>' || c == ',' || c == '[' || c == ']' || c == '*' || c == '&' || c == ' ' || c == '\0') {
            if (!token.empty()) {
                auto mapIt = typeMap.find(token);
                if (mapIt != typeMap.end()) {
                    result += mapIt->second;
                } else {
                    auto dotPos = token.rfind('.');
                    result += (dotPos != std::string::npos) ? token.substr(dotPos + 1) : token;
                }
                token.clear();
            }
            if (c != '\0') result += (c == ',') ? ", " : std::string(1, c);
        } else {
            token += c;
        }
    }
    return result;
}

// IL2CPP type enum
#define IL2CPP_TYPE_GENERICINST 0x15

// Minimal struct layout to read generic type args directly
struct DumperGenericInst {
    uint32_t type_argc;
    void** type_argv; // const Il2CppType**
};

struct DumperGenericContext {
    const DumperGenericInst* class_inst;
    const DumperGenericInst* method_inst;
};

struct DumperGenericClass {
    void* type;                    // type definition
    DumperGenericContext context;
    void* cached_class;
};

struct DumperIl2CppTypeLayout {
    union {
        void* dummy;
        DumperGenericClass* generic_class; // valid when type == IL2CPP_TYPE_GENERICINST
    } data;
    uint32_t bits; // attrs:16, typeEnum:8, num_mods:6, byref:1, pinned:1
};

std::string Dumper::getTypeName(void *type) {
    if (!type) return "unknown";

    auto klass = Variables::IL2CPP::il2cpp_class_from_type(type);

    DumperIl2CppTypeLayout* t = (DumperIl2CppTypeLayout*)type;
    uint32_t typeEnum = (t->bits >> 16) & 0xFF;

    if (typeEnum == IL2CPP_TYPE_GENERICINST && t->data.generic_class && klass) {
        DumperGenericClass* gc = t->data.generic_class;
        if (gc && gc->context.class_inst) {
            const DumperGenericInst* inst = gc->context.class_inst;
            if (inst && inst->type_argc > 0 && inst->type_argc <= 16 && inst->type_argv) {
                const char* rawName = Variables::IL2CPP::il2cpp_class_get_name(klass);
                if (rawName) {
                    std::string name(rawName);
                    auto backtick = name.find('`');
                    if (backtick != std::string::npos) {
                        std::string result = name.substr(0, backtick) + "<";
                        for (uint32_t i = 0; i < inst->type_argc; i++) {
                            if (i > 0) result += ", ";
                            void* argType = inst->type_argv[i];
                            if (argType) {
                                result += getTypeName(argType);
                            } else {
                                result += "unknown";
                            }
                        }
                        result += ">";
                        return result;
                    }
                }
            }
        }
    }

    if (klass) return mapTypeName(getClassName(klass));
    return "unknown";
}

std::string Dumper::getCSafeName(void *klass) {
    if (!klass) return "System_Object";
    const char *name = Variables::IL2CPP::il2cpp_class_get_name(klass);
    const char *ns = Variables::IL2CPP::il2cpp_class_get_namespace(klass);
    if (!name) return "System_Object";

    std::string nameStr(name);
    std::string nsStr = ns ? std::string(ns) : "";

    std::string fullName;
    if (!nsStr.empty()) {
        fullName = nsStr;
        std::replace(fullName.begin(), fullName.end(), '.', '_');
        fullName += "_";
    }

    auto backtickPos = nameStr.find('`');
    if (backtickPos != std::string::npos) {
        nameStr = nameStr.substr(0, backtickPos);
    }
    fullName += nameStr;
    std::string cleaned;
    for (char c : fullName) {
        if (c == '<' || c == '>' || c == ',') cleaned += '_';
        else if (c == ' ') continue;
        else cleaned += c;
    }
    return cleaned;
}

std::string Dumper::getCTypeName(void *type) {
    if (!type) return "void*";

    static const std::unordered_map<std::string, std::string> cTypeMap = {
        {"Void", "void"},
        {"Boolean", "bool"},
        {"Byte", "uint8_t"},
        {"SByte", "int8_t"},
        {"Int16", "int16_t"},
        {"UInt16", "uint16_t"},
        {"Int32", "int32_t"},
        {"UInt32", "uint32_t"},
        {"Int64", "int64_t"},
        {"UInt64", "uint64_t"},
        {"Single", "float"},
        {"Double", "double"},
        {"Char", "uint16_t"},
        {"IntPtr", "intptr_t"},
        {"UIntPtr", "uintptr_t"},
    };

    auto klass = Variables::IL2CPP::il2cpp_class_from_type(type);
    if (!klass) return "void*";

    const char *name = Variables::IL2CPP::il2cpp_class_get_name(klass);
    const char *ns = Variables::IL2CPP::il2cpp_class_get_namespace(klass);
    if (!name) return "void*";

    std::string nameStr(name);
    auto primIt = cTypeMap.find(nameStr);
    if (primIt != cTypeMap.end()) return primIt->second;

    std::string nsStr = ns ? std::string(ns) : "";

    bool isValueType = Variables::IL2CPP::il2cpp_class_is_valuetype(klass);

    std::string fullName;
    if (!nsStr.empty()) {
        fullName = nsStr;
        std::replace(fullName.begin(), fullName.end(), '.', '_');
        fullName += "_";
    }

    auto backtickPos = nameStr.find('`');
    if (backtickPos != std::string::npos) nameStr = nameStr.substr(0, backtickPos);
    fullName += nameStr;

    if (isValueType) {
        return "struct " + fullName + "_o";
    } else {
        return "struct " + fullName + "_o*";
    }
}

char Dumper::getTypeChar(void *type) {
    if (!type) return 'v';
    auto klass = Variables::IL2CPP::il2cpp_class_from_type(type);
    if (!klass) return 'i';
    const char *name = Variables::IL2CPP::il2cpp_class_get_name(klass);
    if (!name) return 'i';
    if (strcmp(name, "Void") == 0) return 'v';
    if (strcmp(name, "Single") == 0) return 'f';
    if (strcmp(name, "Double") == 0) return 'd';
    return 'i';
}

std::string Dumper::dumpCStruct(void *klass) {
    if (!klass) return "";
    std::string safeName = getCSafeName(klass);
    bool isValueType = Variables::IL2CPP::il2cpp_class_is_valuetype(klass);
    bool isEnum = Variables::IL2CPP::il2cpp_class_is_enum(klass);

    std::string baseFieldName;
    auto parent = Variables::IL2CPP::il2cpp_class_get_parent(klass);
    if (parent) {
        std::string pName = getCSafeName(parent);
        if (pName != "System_Object" && pName != "System_ValueType" && pName != "System_Enum") {
            baseFieldName = " : " + pName + "_Fields";
        }
    }

    std::stringstream fieldsOut;
    std::stringstream staticFieldsOut;
    int staticCount = 0;

    void *iter = nullptr;
    while (auto field = Variables::IL2CPP::il2cpp_class_get_fields(klass, &iter)) {
        auto attrs = Variables::IL2CPP::il2cpp_field_get_flags(field);
        const char* fieldName = Variables::IL2CPP::il2cpp_field_get_name(field);
        std::string safeFName = fieldName ? fieldName : "unknown";
        for (char& c : safeFName) {
            if (!isalnum(c) && c != '_') c = '_';
        }
        if (safeFName.empty() || isdigit(safeFName[0])) safeFName = "_" + safeFName;

        auto fieldType = Variables::IL2CPP::il2cpp_field_get_type(field);
        std::string cTypeName = fieldType ? getCTypeName(fieldType) : "void*";

        if (attrs & FIELD_ATTRIBUTE_STATIC) {
            staticFieldsOut << "\t" << cTypeName << " " << safeFName << ";\n";
            staticCount++;
        } else {
            fieldsOut << "\t" << cTypeName << " " << safeFName << ";\n";
        }
    }

    std::stringstream out;
    if (isEnum || isValueType) {
        out << "struct " << safeName << "_Fields {\n";
        out << fieldsOut.str();
        out << "};\n";
    } else {
        out << "struct " << safeName << "_Fields" << baseFieldName << " {\n";
        out << fieldsOut.str();
        out << "};\n";
    }

    out << "struct " << safeName << "_VTable {\n};\n";
    out << "struct " << safeName << "_c {\n";
    out << "\tIl2CppClass_1 _1;\n";
    if (staticCount > 0) out << "\tstruct " << safeName << "_StaticFields* static_fields;\n";
    else out << "\tvoid* static_fields;\n";
    out << "\tIl2CppRGCTXData* rgctx_data;\n";
    out << "\tIl2CppClass_2 _2;\n";
    out << "\t" << safeName << "_VTable vtable;\n";
    out << "};\n";

    out << "struct " << safeName << "_o {\n";
    if (isValueType || isEnum) {
        out << "\t" << safeName << "_Fields fields;\n";
    } else {
        out << "\t" << safeName << "_c *klass;\n";
        out << "\tvoid *monitor;\n";
        out << "\t" << safeName << "_Fields fields;\n";
    }
    out << "};\n";

    if (staticCount > 0) {
        out << "struct " << safeName << "_StaticFields {\n";
        out << staticFieldsOut.str();
        out << "};\n";
    }

    return out.str();
}

void Dumper::Log(const char *fmt, ...) {
#if DEBUG
    File logfile(dumpDir + "/logs.txt", "a");
    if (!logfile.ok()) return;
    va_list args;
    va_start(args, fmt);
    vfprintf(logfile, fmt, args);
    fprintf(logfile, "\n");
    va_end(args);
    logfile.close();
#endif
}

std::string Dumper::uint16ToString(uint16_t *str) {
    std::string out;
    if (!str) return out;
    while (*str) {
        uint16_t c = *str;
        if (c >= 32 && c <= 126 && c != '"' && c != '\\') {
            out += (char)c;
        } else {
            std::stringstream ss;
            ss << "\\\\u" << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << c;
            out += ss.str();
        }
        str++;
    }
    return out;
}

std::string Dumper::toHexUnicode(char c) {
    std::stringstream ss;
    ss << std::hex << std::uppercase << std::setw(4) << std::setfill('0') << (int)(unsigned char)c;
    return ss.str();
}

std::string Dumper::convertNonAlnumToHexUnicode(const std::string &input) {
    std::string out;
    for (char c : input) {
        if (isalnum(c) || c == '_' || c == '.') {
            out += c;
        } else {
            out += "\\\\u" + toHexUnicode(c);
        }
    }
    return out;
}

void Dumper::GenScript::init() {
    jsonData["ScriptMethod"] = json::array();
    jsonData["ScriptString"] = json::array();
    jsonData["ScriptMetadata"] = json::array();
    jsonData["ScriptMetadataMethod"] = json::array();
    scriptFile.open(dumpDir + "/script.json", "w");
}

void Dumper::GenScript::save() {
    Log("Saving Script...");
    if (!scriptFile.ok()) return;
    std::string data = jsonData.dump(4, ' ', true);

    std::string outData;
    outData.reserve(data.size());
    size_t pos = 0;
    size_t nextPos;
    while ((nextPos = data.find("\\\\\\\\u", pos)) != std::string::npos) {
        outData.append(data, pos, nextPos - pos);
        outData.append("\\u");
        pos = nextPos + 5;
    }
    outData.append(data, pos, std::string::npos);
    data = std::move(outData);

    scriptFile.write(data.c_str());
    Log("Script Saved.");
}

void Dumper::GenScript::addMethod(uint64_t addr, std::string namespaze, std::string klass, std::string method, void *methodInfo) {
    if (dataOffsets.find(addr) != dataOffsets.end()) return;

    std::string signature;
    std::string typeSig;

    if (methodInfo) {
        auto returnType = Variables::IL2CPP::il2cpp_method_get_return_type(methodInfo);
        signature += getCTypeName(returnType) + " ";
        typeSig += getTypeChar(returnType);
    } else {
        signature += "void ";
        typeSig += "v";
    }

    std::string sigFuncName;
    if (!namespaze.empty()) {
        sigFuncName = namespaze;
        std::replace(sigFuncName.begin(), sigFuncName.end(), '.', '_');
        sigFuncName += "_";
    }
    std::string klassClean = klass;
    auto angleBracket = klassClean.find('<');
    if (angleBracket != std::string::npos) klassClean = klassClean.substr(0, angleBracket);
    sigFuncName += klassClean + "__" + method;
    signature += sigFuncName + " (";

    int32_t paramCount = methodInfo ? Variables::IL2CPP::il2cpp_method_get_param_count(methodInfo) : 0;
    for (int32_t i = 0; i < paramCount; i++) {
        auto param = Variables::IL2CPP::il2cpp_method_get_param(methodInfo, i);
        const char *paramName = Variables::IL2CPP::il2cpp_method_get_param_name(methodInfo, i);
        signature += getCTypeName(param) + " " + (paramName ? paramName : "");
        typeSig += getTypeChar(param);
        if (i < paramCount - 1) signature += ", ";
    }

    if (paramCount > 0) signature += ", ";
    signature += "const MethodInfo* method);";
    typeSig += 'i';

    namespaze = convertNonAlnumToHexUnicode(namespaze);
    method = convertNonAlnumToHexUnicode(method);
    klass = convertNonAlnumToHexUnicode(klass);
    std::string fullName;
    if (namespaze.empty()) fullName = klass + "$$" + method;
    else fullName = namespaze + "." + klass + "$$" + method;

    jsonData["ScriptMethod"].push_back({{"Address", addr}, {"Name", fullName}, {"Signature", signature}, {"TypeSignature", typeSig}});
    dataOffsets.insert(addr);
}

void Dumper::GenScript::addMetadataMethod(void *methodInfo, uint64_t methodAddr, const std::string& namespaze, const std::string& klassName, const std::string& methodName) {
    if (!methodInfo) return;
    uint64_t addr = (uint64_t)methodInfo - Variables::info.address;

    std::string name = "Method$";
    if (!namespaze.empty()) name += namespaze + ".";
    name += klassName + "." + methodName + "()";

    std::string safeName = convertNonAlnumToHexUnicode(name);
    jsonData["ScriptMetadataMethod"].push_back({{"Address", addr}, {"Name", safeName}, {"MethodAddress", methodAddr}});
}

void Dumper::GenScript::addString(uint64_t addr, const std::string& value) {
    jsonData["ScriptString"].push_back({{"Address", addr}, {"Value", value}});
}
