using Org.LLRP.LTK.LLRPV1;
using Org.LLRP.LTK.LLRPV1.DataType;
using Org.LLRP.LTK.LLRPV1.Impinj;
using System;
using System.Collections.Generic;

namespace SecuCodeApp
{
    static class ReaderConfig
    {
        public static Result<MSG_IMPINJ_ENABLE_EXTENSIONS_RESPONSE> Enable_ImpinjExtensions(this LLRPClient reader)
        {
            var message = new MSG_IMPINJ_ENABLE_EXTENSIONS();
            var response = reader.CUSTOM_MESSAGE(message, out var error, 2000) as MSG_IMPINJ_ENABLE_EXTENSIONS_RESPONSE;
            return new Result<MSG_IMPINJ_ENABLE_EXTENSIONS_RESPONSE>(response, error);
        }

        public static Result<MSG_IMPINJ_ENABLE_EXTENSIONS_RESPONSE> Enable_EventsAndReports(this LLRPClient reader)
        {
            var message = new MSG_ENABLE_EVENTS_AND_REPORTS();
            reader.ENABLE_EVENTS_AND_REPORTS(message, out var error, 2000);
            return new Result<MSG_IMPINJ_ENABLE_EXTENSIONS_RESPONSE>(null, error);
        }

        public static Result<MSG_ADD_ACCESSSPEC_RESPONSE>
            Add_ContinuousAccessSpec(this LLRPClient reader, PARAM_C1G2TargetTag target, List<IParameter> accessSpecOpList)
        {
            var accessSpec = new PARAM_AccessSpec()
            {
                AccessSpecID = 431,
                AntennaID = 1,
                ProtocolID = ENUM_AirProtocols.EPCGlobalClass1Gen2,
                CurrentState = ENUM_AccessSpecState.Disabled,
                ROSpecID = 1,

                AccessSpecStopTrigger = new PARAM_AccessSpecStopTrigger()
                {
                    AccessSpecStopTrigger = ENUM_AccessSpecStopTriggerType.Null,
                    OperationCountValue = 1
                },

                AccessCommand = new PARAM_AccessCommand()
                {
                    AirProtocolTagSpec = new UNION_AirProtocolTagSpec(),
                    AccessCommandOpSpec = new UNION_AccessCommandOpSpec()
                },

                AccessReportSpec = new PARAM_AccessReportSpec()
                {
                    AccessReportTrigger = ENUM_AccessReportTriggerType.Whenever_ROReport_Is_Generated
                }
            };

            foreach (var accessSpecOp in accessSpecOpList)
            {
                accessSpec.AccessCommand.AccessCommandOpSpec.Add(accessSpecOp);
            }
            accessSpec.AccessCommand.AirProtocolTagSpec.Add(new PARAM_C1G2TagSpec()
            {
                C1G2TargetTag = new PARAM_C1G2TargetTag[] { target },
            });

            var message = new MSG_ADD_ACCESSSPEC() { AccessSpec = accessSpec };
            var response = reader.ADD_ACCESSSPEC(message, out var error, 2000);

            return new Result<MSG_ADD_ACCESSSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_ADD_ACCESSSPEC_RESPONSE>
            Add_SingleAccessSpec(this LLRPClient reader, PARAM_C1G2TargetTag target, List<IParameter> accessSpecOpList)
        {
            var accessSpec = new PARAM_AccessSpec()
            {
                AccessSpecID = 431,
                AntennaID = 1,
                ProtocolID = ENUM_AirProtocols.EPCGlobalClass1Gen2,
                CurrentState = ENUM_AccessSpecState.Disabled,
                ROSpecID = 1,

                AccessSpecStopTrigger = new PARAM_AccessSpecStopTrigger()
                {
                    AccessSpecStopTrigger = ENUM_AccessSpecStopTriggerType.Operation_Count,
                    OperationCountValue = 1,
                },

                AccessCommand = new PARAM_AccessCommand()
                {
                    AirProtocolTagSpec = new UNION_AirProtocolTagSpec(),
                    AccessCommandOpSpec = new UNION_AccessCommandOpSpec()
                },

                AccessReportSpec = new PARAM_AccessReportSpec()
                {
                    AccessReportTrigger = ENUM_AccessReportTriggerType.End_Of_AccessSpec
                }
            };

            foreach (var accessSpecOp in accessSpecOpList)
            {
                accessSpec.AccessCommand.AccessCommandOpSpec.Add(accessSpecOp);
            }
            accessSpec.AccessCommand.AirProtocolTagSpec.Add(new PARAM_C1G2TagSpec()
            {
                C1G2TargetTag = new PARAM_C1G2TargetTag[] { target },
            });

            var message = new MSG_ADD_ACCESSSPEC() { AccessSpec = accessSpec };
            var response = reader.ADD_ACCESSSPEC(message, out var error, 2000);

            return new Result<MSG_ADD_ACCESSSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_ENABLE_ACCESSSPEC_RESPONSE> Enable_AccessSpec(this LLRPClient reader)
        {
            var message = new MSG_ENABLE_ACCESSSPEC()
            {
                AccessSpecID = 431
            };

            var response = reader.ENABLE_ACCESSSPEC(message, out var error, 2000);
            return new Result<MSG_ENABLE_ACCESSSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_DELETE_ACCESSSPEC_RESPONSE> Delete_AccessSpec(this LLRPClient reader)
        {
            var message = new MSG_DELETE_ACCESSSPEC()
            {
                AccessSpecID = 431
            };

            var response = reader.DELETE_ACCESSSPEC(message, out var error, 2000);
            return new Result<MSG_DELETE_ACCESSSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_ADD_ROSPEC_RESPONSE> Add_RoSpec(this LLRPClient reader, TimeSpan? timeout = null)
        {
            var startTrigger = new PARAM_ROSpecStartTrigger { ROSpecStartTriggerType = ENUM_ROSpecStartTriggerType.Immediate };
            var stopTrigger = new PARAM_ROSpecStopTrigger { ROSpecStopTriggerType = ENUM_ROSpecStopTriggerType.Null };

            if (timeout != null)
            {
                startTrigger.ROSpecStartTriggerType = ENUM_ROSpecStartTriggerType.Null;

                stopTrigger.ROSpecStopTriggerType = ENUM_ROSpecStopTriggerType.Duration;
                stopTrigger.DurationTriggerValue = (uint)timeout.Value.TotalMilliseconds;
            }

            var ROSpec = new PARAM_ROSpec()
            {
                // ROSpec must be disabled by default
                CurrentState = ENUM_ROSpecState.Disabled,
                ROSpecID = 1,

                // Specifies the start and stop triggers for the ROSpec
                ROBoundarySpec = new PARAM_ROBoundarySpec()
                {
                    // The reader will start reading tags as soon as the is enabled
                    ROSpecStartTrigger = new PARAM_ROSpecStartTrigger()
                    {
                        ROSpecStartTriggerType = ENUM_ROSpecStartTriggerType.Immediate
                    },

                    ROSpecStopTrigger = stopTrigger
                },

                SpecParameter = new UNION_SpecParameter(),
            };

            ROSpec.SpecParameter.Add(WispInventoryAISpec());

            var message = new MSG_ADD_ROSPEC() { ROSpec = ROSpec };
            var response = reader.ADD_ROSPEC(message, out var error, 2000);

            return new Result<MSG_ADD_ROSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_ENABLE_ROSPEC_RESPONSE> Enable_RoSpec(this LLRPClient reader, uint id)
        {
            var message = new MSG_ENABLE_ROSPEC() { ROSpecID = id };
            var response = reader.ENABLE_ROSPEC(message, out var error, 2000);

            return new Result<MSG_ENABLE_ROSPEC_RESPONSE>(response, error);
        }

        public static Result<MSG_DELETE_ROSPEC_RESPONSE> Delete_RoSpec(this LLRPClient reader, uint id)
        {
            var message = new MSG_DELETE_ROSPEC() { ROSpecID = id };
            var response = reader.DELETE_ROSPEC(message, out var error, 2000);

            return new Result<MSG_DELETE_ROSPEC_RESPONSE>(response, error);
        }

        private static PARAM_AISpec WispInventoryAISpec()
        {
            var antennaConfig = new PARAM_AntennaConfiguration()
            {
                AntennaID = 1,
                AirProtocolInventoryCommandSettings = new UNION_AirProtocolInventoryCommandSettings(),
            };
            var inventoryCommand = new PARAM_C1G2InventoryCommand()
            {
                C1G2RFControl = new PARAM_C1G2RFControl() { ModeIndex = 0, Tari = 0 },
                C1G2SingulationControl = new PARAM_C1G2SingulationControl() { Session = new TwoBits(1), TagPopulation = 1 },
                TagInventoryStateAware = false,
            };
            antennaConfig.AirProtocolInventoryCommandSettings.Add(inventoryCommand);

            var enabledAntennas = new UInt16Array();
            enabledAntennas.Add(1);

            return new PARAM_AISpec()
            {
                AntennaIDs = enabledAntennas,

                AISpecStopTrigger = new PARAM_AISpecStopTrigger()
                {
                    AISpecStopTriggerType = ENUM_AISpecStopTriggerType.Null
                },

                InventoryParameterSpec = new PARAM_InventoryParameterSpec[]
                {
                    new PARAM_InventoryParameterSpec()
                    {
                        InventoryParameterSpecID = 1234,
                        ProtocolID = ENUM_AirProtocols.EPCGlobalClass1Gen2,
                        AntennaConfiguration = new PARAM_AntennaConfiguration[] { antennaConfig },
                    }
                }
            };
        }
    }
}
