/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * BP4FileReader.tcc
 *
 *  Created on: Aug 1, 2018
 *      Author: Lipeng Wan wanl@ornl.gov
 */

#ifndef ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_
#define ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_

#include "BP4FileReader.h"

#include "adios2/helper/adiosFunctions.h"

namespace adios2
{
namespace core
{
namespace engine
{

template <>
inline void BP4FileReader::GetSyncCommon(Variable<std::string> &variable,
                                        std::string *data)
{
    m_BP4Deserializer.GetValueFromMetadata(variable, data);
}

template <class T>
inline void BP4FileReader::GetSyncCommon(Variable<T> &variable, T *data)
{
    if (variable.m_SingleValue)
    {
        m_BP4Deserializer.GetValueFromMetadata(variable, data);
        return;
    }

    typename Variable<T>::Info &blockInfo =
        m_BP4Deserializer.InitVariableBlockInfo(variable, data);
    m_BP4Deserializer.SetVariableBlockInfo(variable, blockInfo);
    ReadVariableBlocks(variable);
    variable.m_BlocksInfo.clear();
}

template <class T>
void BP4FileReader::GetDeferredCommon(Variable<T> &variable, T *data)
{
    // cheap
    if (variable.m_SingleValue)
    {
        m_BP4Deserializer.GetValueFromMetadata(variable, data);
        return;
    }

    // returns immediately without populating data
    m_BP4Deserializer.InitVariableBlockInfo(variable, data);
    m_BP4Deserializer.m_DeferredVariables.insert(variable.m_Name);
}

template <class T>
void BP4FileReader::ReadVariableBlocks(Variable<T> &variable)
{
    const bool profile = m_BP4Deserializer.m_Profiler.IsActive;

    for (typename Variable<T>::Info &blockInfo : variable.m_BlocksInfo)
    {
        T *originalBlockData = blockInfo.Data;

        for (const auto &stepPair : blockInfo.StepBlockSubStreamsInfo)
        {
            const std::vector<helper::SubStreamBoxInfo> &subStreamsInfo =
                stepPair.second;

            for (const helper::SubStreamBoxInfo &subStreamInfo : subStreamsInfo)
            {
                const size_t subFileIndex = subStreamInfo.SubStreamID;

                if (m_SubFileManager.m_Transports.count(subFileIndex) == 0)
                {
                    const std::string subFile(
                        m_BP4Deserializer.GetBPSubFileName(m_Name,
                                                           subFileIndex));

                    m_SubFileManager.OpenFileID(
                        subFile, subFileIndex, Mode::Read,
                        {{"transport", "File"}}, profile);
                }

                // need to decompress before into m_Memory
                if (subStreamInfo.OperationsInfo.size() > 0)
                {
                    const bool identity =
                        m_BP4Deserializer.IdentityOperation<T>(
                            blockInfo.Operations);

                    const helper::BlockOperationInfo &blockOperationInfo =
                        m_BP4Deserializer.InitPostOperatorBlockData(
                            subStreamInfo.OperationsInfo,
                            variable.m_RawMemory[1], identity);

                    // if identity is true, just read the entire block content
                    char *output =
                        identity ? reinterpret_cast<char *>(blockInfo.Data)
                                 : variable.m_RawMemory[1].data();
                    m_SubFileManager.ReadFile(
                        output, blockOperationInfo.PayloadSize,
                        blockOperationInfo.PayloadOffset, subFileIndex);
                    if (identity)
                    {
                        continue;
                    }
                    m_BP4Deserializer.GetPreOperatorBlockData(
                        variable.m_RawMemory[1], blockOperationInfo,
                        variable.m_RawMemory[0]);
                }
                else
                {
                    const size_t payloadStart = subStreamInfo.Seeks.first;
                    const size_t payloadSize =
                        subStreamInfo.Seeks.second - subStreamInfo.Seeks.first;
                    // a single m_Memory can prevent threading per variable,
                    // need to think for later
                    variable.m_RawMemory[0].resize(payloadSize);
                    m_SubFileManager.ReadFile(variable.m_RawMemory[0].data(),
                                              payloadSize, payloadStart,
                                              subFileIndex);
                }
                m_BP4Deserializer.ClipContiguousMemory<T>(
                    blockInfo, variable.m_RawMemory[0], subStreamInfo.BlockBox,
                    subStreamInfo.IntersectionBox);
            }
            // advance pointer to next step
            blockInfo.Data += helper::GetTotalSize(blockInfo.Count);
        }
        blockInfo.Data = originalBlockData;
    }
}

} // end namespace engine
} // end namespace core
} // end namespace adios2

#endif /* ADIOS2_ENGINE_BP4_BP4FILEREADER_TCC_ */
