/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4Deserializer.cpp
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#include "BP4Deserializer.h"
#include "BP4Deserializer.tcc"

#include <future>
#include <unordered_set>
#include <vector>

#include "adios2/helper/adiosFunctions.h" //helper::ReadValue<T>

#ifdef _WIN32
#pragma warning(disable : 4503) // Windows complains about SubFileInfoMap levels
#endif

namespace adios2
{
namespace format
{

std::mutex BP4Deserializer::m_Mutex;

BP4Deserializer::BP4Deserializer(MPI_Comm mpiComm, const bool debugMode)
: BP4Base(mpiComm, debugMode)
{
}

void BP4Deserializer::ParseMetadata(const BufferSTL &bufferSTL, core::IO &io)
{
    ParseMinifooter(bufferSTL);
    ParsePGIndex(bufferSTL, io);
    ParseVariablesIndex(bufferSTL, io);
    ParseAttributesIndex(bufferSTL, io);
}

const helper::BlockOperationInfo &BP4Deserializer::InitPostOperatorBlockData(
    const std::vector<helper::BlockOperationInfo> &blockOperationsInfo,
    std::vector<char> &postOpData, const bool identity) const
{

    size_t index = 0;
    for (const helper::BlockOperationInfo &blockOperationInfo :
         blockOperationsInfo)
    {
        const std::string type = blockOperationInfo.Info.at("Type");
        if (m_TransformTypes.count(type) == 1)
        {
            if (!identity)
            {
                postOpData.resize(blockOperationInfo.PayloadSize);
            }
            break;
        }
        ++index;
    }
    return blockOperationsInfo.at(index);
}

void BP4Deserializer::GetPreOperatorBlockData(
    const std::vector<char> &postOpData,
    const helper::BlockOperationInfo &blockOperationInfo,
    std::vector<char> &preOpData) const
{
    // pre-allocate decompressed block
    preOpData.resize(helper::GetTotalSize(blockOperationInfo.PreCount) *
                     blockOperationInfo.PreSizeOf);

    // get the right bp4Op
    std::shared_ptr<BP4Operation> bp4Op =
        SetBP4Operation(blockOperationInfo.Info.at("Type"));
    bp4Op->GetData(postOpData.data(), blockOperationInfo, preOpData.data());
}

// PRIVATE
void BP4Deserializer::ParseMinifooter(const BufferSTL &bufferSTL)
{
    auto lf_GetEndianness = [](const uint8_t endianness, bool &isLittleEndian) {

        switch (endianness)
        {
        case 0:
            isLittleEndian = true;
            break;
        case 1:
            isLittleEndian = false;
            break;
        }
    };

    const auto &buffer = bufferSTL.m_Buffer;
    const size_t bufferSize = buffer.size();
    size_t position = bufferSize - 4;
    const uint8_t endianess = helper::ReadValue<uint8_t>(buffer, position);
    lf_GetEndianness(endianess, m_Minifooter.IsLittleEndian);
    position += 1;

    const uint8_t subFilesIndex = helper::ReadValue<uint8_t>(buffer, position);
    if (subFilesIndex > 0)
    {
        m_Minifooter.HasSubFiles = true;
    }

    m_Minifooter.Version = helper::ReadValue<uint8_t>(buffer, position);
    if (m_Minifooter.Version < 3)
    {
        throw std::runtime_error("ERROR: ADIOS2 only supports bp format "
                                 "version 3 and above, found " +
                                 std::to_string(m_Minifooter.Version) +
                                 " version \n");
    }

    position = bufferSize - m_MetadataSet.MiniFooterSize;

    m_Minifooter.VersionTag.assign(&buffer[position], 28);
    position += 28;

    m_Minifooter.PGIndexStart = helper::ReadValue<uint64_t>(buffer, position);
    m_Minifooter.VarsIndexStart = helper::ReadValue<uint64_t>(buffer, position);
    m_Minifooter.AttributesIndexStart =
        helper::ReadValue<uint64_t>(buffer, position);
}

void BP4Deserializer::ParsePGIndex(const BufferSTL &bufferSTL,
                                   const core::IO &io)
{
    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.PGIndexStart;

    m_MetadataSet.DataPGCount = helper::ReadValue<uint64_t>(buffer, position);
    const size_t length = helper::ReadValue<uint64_t>(buffer, position);

    size_t localPosition = 0;

    std::unordered_set<uint32_t> stepsFound;
    m_MetadataSet.StepsCount = 0;

    while (localPosition < length)
    {
        ProcessGroupIndex index = ReadProcessGroupIndexHeader(buffer, position);
        if (index.IsColumnMajor == 'y')
        {
            m_IsRowMajor = false;
        }

        m_MetadataSet.CurrentStep = static_cast<size_t>(index.Step - 1);

        // Count the number of unseen steps
        if (stepsFound.insert(index.Step).second)
        {
            ++m_MetadataSet.StepsCount;
        }

        localPosition += index.Length + 2;
    }

    if (m_IsRowMajor != helper::IsRowMajor(io.m_HostLanguage))
    {
        m_ReverseDimensions = true;
    }
}

void BP4Deserializer::ParseVariablesIndex(const BufferSTL &bufferSTL,
                                          core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {

        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

        case (type_string):
        {
            DefineVariableInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_byte):
        {
            DefineVariableInIO<signed char>(header, io, buffer, position);
            break;
        }

        case (type_short):
        {
            DefineVariableInIO<short>(header, io, buffer, position);
            break;
        }

        case (type_integer):
        {
            DefineVariableInIO<int>(header, io, buffer, position);
            break;
        }

        case (type_long):
        {
            DefineVariableInIO<int64_t>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_byte):
        {
            DefineVariableInIO<unsigned char>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_short):
        {
            DefineVariableInIO<unsigned short>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_integer):
        {
            DefineVariableInIO<unsigned int>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_long):
        {
            DefineVariableInIO<uint64_t>(header, io, buffer, position);
            break;
        }

        case (type_real):
        {
            DefineVariableInIO<float>(header, io, buffer, position);
            break;
        }

        case (type_double):
        {
            DefineVariableInIO<double>(header, io, buffer, position);
            break;
        }

        case (type_long_double):
        {
            DefineVariableInIO<long double>(header, io, buffer, position);
            break;
        }

        case (type_complex):
        {
            DefineVariableInIO<std::complex<float>>(header, io, buffer,
                                                    position);
            break;
        }

        case (type_double_complex):
        {
            DefineVariableInIO<std::complex<double>>(header, io, buffer,
                                                     position);
            break;
        }

        case (type_long_double_complex):
        {
            DefineVariableInIO<std::complex<long double>>(header, io, buffer,
                                                          position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.VarsIndexStart;

    const uint32_t count = helper::ReadValue<uint32_t>(buffer, position);
    const uint64_t length = helper::ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;

    if (m_Threads == 1)
    {
        while (localPosition < length)
        {
            lf_ReadElementIndex(io, buffer, position);

            const size_t elementIndexSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(buffer, position));
            position += elementIndexSize;
            localPosition = position - startPosition;
        }
        return;
    }

    // threads for reading Variables
    std::vector<std::future<void>> asyncs(m_Threads);
    std::vector<size_t> asyncPositions(m_Threads);

    bool launched = false;

    while (localPosition < length)
    {
        // extract async positions
        for (unsigned int t = 0; t < m_Threads; ++t)
        {
            asyncPositions[t] = position;
            const size_t elementIndexSize = static_cast<size_t>(
                helper::ReadValue<uint32_t>(buffer, position));
            position += elementIndexSize;
            localPosition = position - startPosition;

            if (launched)
            {
                asyncs[t].get();
            }

            if (localPosition <= length)
            {
                asyncs[t] = std::async(std::launch::async, lf_ReadElementIndex,
                                       std::ref(io), std::ref(buffer),
                                       asyncPositions[t]);
            }
        }
        launched = true;
    }

    for (auto &async : asyncs)
    {
        if (async.valid())
        {
            async.wait();
        }
    }
}

void BP4Deserializer::ParseAttributesIndex(const BufferSTL &bufferSTL,
                                           core::IO &io)
{
    auto lf_ReadElementIndex = [&](
        core::IO &io, const std::vector<char> &buffer, size_t position) {

        const ElementIndexHeader header =
            ReadElementIndexHeader(buffer, position);

        switch (header.DataType)
        {

        case (type_string):
        {
            DefineAttributeInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_string_array):
        {
            DefineAttributeInIO<std::string>(header, io, buffer, position);
            break;
        }

        case (type_byte):
        {
            DefineAttributeInIO<signed char>(header, io, buffer, position);
            break;
        }

        case (type_short):
        {
            DefineAttributeInIO<short>(header, io, buffer, position);
            break;
        }

        case (type_integer):
        {
            DefineAttributeInIO<int>(header, io, buffer, position);
            break;
        }

        case (type_long):
        {
            DefineAttributeInIO<int64_t>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_byte):
        {
            DefineAttributeInIO<unsigned char>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_short):
        {
            DefineAttributeInIO<unsigned short>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_integer):
        {
            DefineAttributeInIO<unsigned int>(header, io, buffer, position);
            break;
        }

        case (type_unsigned_long):
        {
            DefineAttributeInIO<uint64_t>(header, io, buffer, position);
            break;
        }

        case (type_real):
        {
            DefineAttributeInIO<float>(header, io, buffer, position);
            break;
        }

        case (type_double):
        {
            DefineAttributeInIO<double>(header, io, buffer, position);
            break;
        }

        case (type_long_double):
        {
            DefineAttributeInIO<long double>(header, io, buffer, position);
            break;
        }

        } // end switch
    };

    const auto &buffer = bufferSTL.m_Buffer;
    size_t position = m_Minifooter.AttributesIndexStart;

    const uint32_t count = helper::ReadValue<uint32_t>(buffer, position);
    const uint64_t length = helper::ReadValue<uint64_t>(buffer, position);

    const size_t startPosition = position;
    size_t localPosition = 0;

    // Read sequentially
    while (localPosition < length)
    {
        lf_ReadElementIndex(io, buffer, position);
        const size_t elementIndexSize =
            static_cast<size_t>(helper::ReadValue<uint32_t>(buffer, position));
        position += elementIndexSize;
        localPosition = position - startPosition;
    }
}

std::map<std::string, helper::SubFileInfoMap>
BP4Deserializer::PerformGetsVariablesSubFileInfo(core::IO &io)
{
    if (m_DeferredVariablesMap.empty())
    {
        return m_DeferredVariablesMap;
    }

    for (auto &subFileInfoPair : m_DeferredVariablesMap)
    {
        const std::string variableName(subFileInfoPair.first);
        const std::string type(io.InquireVariableType(variableName));

        if (type == "compound")
        {
        }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        subFileInfoPair.second =                                               \
            GetSubFileInfo(*io.InquireVariable<T>(variableName));              \
    }
        ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
    }
    return m_DeferredVariablesMap;
}

void BP4Deserializer::ClipMemory(const std::string &variableName, core::IO &io,
                                 const std::vector<char> &contiguousMemory,
                                 const Box<Dims> &blockBox,
                                 const Box<Dims> &intersectionBox) const
{
    const std::string type(io.InquireVariableType(variableName));

    if (type == "compound")
    {
    }
#define declare_type(T)                                                        \
    else if (type == helper::GetType<T>())                                     \
    {                                                                          \
        core::Variable<T> *variable = io.InquireVariable<T>(variableName);     \
        if (variable != nullptr)                                               \
        {                                                                      \
            helper::ClipContiguousMemory(variable->m_Data, variable->m_Start,  \
                                         variable->m_Count, contiguousMemory,  \
                                         blockBox, intersectionBox,            \
                                         m_IsRowMajor, m_ReverseDimensions);   \
        }                                                                      \
    }
    ADIOS2_FOREACH_TYPE_1ARG(declare_type)
#undef declare_type
}

#define declare_template_instantiation(T)                                      \
    template void BP4Deserializer::GetSyncVariableDataFromStream(              \
        core::Variable<T> &, BufferSTL &) const;                               \
                                                                               \
    template typename core::Variable<T>::Info &                                \
    BP4Deserializer::InitVariableBlockInfo(core::Variable<T> &, T *);          \
                                                                               \
    template void BP4Deserializer::SetVariableBlockInfo(                       \
        core::Variable<T> &, typename core::Variable<T>::Info &);              \
                                                                               \
    template void BP4Deserializer::ClipContiguousMemory<T>(                    \
        typename core::Variable<T>::Info &, const std::vector<char> &,         \
        const Box<Dims> &, const Box<Dims> &) const;                           \
                                                                               \
    template void BP4Deserializer::GetValueFromMetadata(                       \
        core::Variable<T> &variable, T *) const;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

#define declare_template_instantiation(T)                                      \
                                                                               \
    template std::map<std::string, helper::SubFileInfoMap>                     \
    BP4Deserializer::GetSyncVariableSubFileInfo(const core::Variable<T> &)     \
        const;                                                                 \
                                                                               \
    template void BP4Deserializer::GetDeferredVariable(core::Variable<T> &,    \
                                                       T *);                   \
                                                                               \
    template helper::SubFileInfoMap BP4Deserializer::GetSubFileInfo(           \
        const core::Variable<T> &) const;                                      \
                                                                               \
    template std::map<size_t, std::vector<typename core::Variable<T>::Info>>   \
    BP4Deserializer::AllStepsBlocksInfo(const core::Variable<T> &) const;      \
                                                                               \
    template std::vector<typename core::Variable<T>::Info>                     \
    BP4Deserializer::BlocksInfo(const core::Variable<T> &, const size_t)       \
        const;                                                                 \
                                                                               \
    template bool BP4Deserializer::IdentityOperation<T>(                       \
        const std::vector<typename core::Variable<T>::Operation> &)            \
        const noexcept;

ADIOS2_FOREACH_TYPE_1ARG(declare_template_instantiation)
#undef declare_template_instantiation

} // end namespace format
} // end namespace adios2
