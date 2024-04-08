// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.


#ifndef INCLUDED_OCIO_MERGE_CONFIG_SECTION_MERGER_H
#define INCLUDED_OCIO_MERGE_CONFIG_SECTION_MERGER_H

#include <string>
#include <unordered_map>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>

#include "utils/StringUtils.h"
#include "Logging.h"


namespace OCIO_NAMESPACE
{

struct MergeHandlerOptions
{
    const ConstConfigRcPtr & baseConfig;
    const ConstConfigRcPtr & inputConfig;
    const ConfigMergingParametersRcPtr & params;
    ConfigRcPtr & mergedConfig;
};

class SectionMerger
{
public:

    enum MergerFlag
    {
        MERGERSTATE_NOFLAG,
        MERGERSTATE_ADD_INITIAL_CONFIG,
    };

    SectionMerger(MergeHandlerOptions options)
        : m_baseConfig(options.baseConfig), m_inputConfig(options.inputConfig), 
          m_mergedConfig(options.mergedConfig), m_params(options.params)
    {
        m_strategy = options.params->getDefaultStrategy();
        m_params->setDefaultStrategy(options.params->getDefaultStrategy());
    }
    
    void merge()
    {
        switch (m_strategy)
        {
            case ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_INPUT:
                handlePreferInput();
                break;
            case ConfigMergingParameters::MergeStrategies::STRATEGY_PREFER_BASE:
                handlePreferBase();
                break;
            case ConfigMergingParameters::MergeStrategies::STRATEGY_INPUT_ONLY:
                handleInputOnly();
                break;
            case ConfigMergingParameters::MergeStrategies::STRATEGY_BASE_ONLY:
                handleBaseOnly();
                break;
            case ConfigMergingParameters::MergeStrategies::STRATEGY_REMOVE:
                handleRemove();
                break;
            case ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET:
                // nothing to do.
                break;
            default:
                break;
        }
    }

    void notify(std::string s, bool mustThrow) const;

private:
    virtual void handlePreferInput()
    {
        LogWarning(getName() + std::string(" section does not supported strategy 'PreferInput'"));
    }

    virtual void handlePreferBase()
    {
        LogWarning(getName() + std::string(" section does not supported strategy 'PreferBase'"));
    }

    virtual void handleInputOnly()
    {
        LogWarning(getName() + std::string(" section does not supported strategy 'InputOnly'"));
    }

    virtual void handleBaseOnly()
    {
        LogWarning(getName() + std::string(" section does not supported strategy 'BaseOnly'"));
    }

    virtual void handleRemove()
    {
        LogWarning(getName() + std::string(" section does not supported strategy 'Remove'"));
    }

protected:
    const ConstConfigRcPtr & m_baseConfig;
    const ConstConfigRcPtr & m_inputConfig;
    ConfigRcPtr & m_mergedConfig;
    
    const ConfigMergingParametersRcPtr & m_params;
    ConfigMergingParameters::MergeStrategies m_strategy;

    void mergeStringVecWithoutDuplicate(StringUtils::StringVec & input, 
                                        StringUtils::StringVec & mergedVec) const;
    void inputFirstMergeStringVec(StringUtils::StringVec & input, 
                                 StringUtils::StringVec & base, 
                                 StringUtils::StringVec & mergedVec) const;

private:
    virtual const std::string getName() const = 0;
};

class GeneralMerger : public SectionMerger
{
public:
    GeneralMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        // This merger always use the default strategy as this is for properties
        // that are not linked to a specific section.
        m_strategy = options.params->getDefaultStrategy();
    }

private:
    const std::string getName() const { return "Miscellaneous"; }

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
};

class RolesMerger : public SectionMerger
{
public:
    RolesMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        ConfigMergingParameters::MergeStrategies strat = options.params->getRoles();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }
    }

private:
    const std::string getName() const { return "Roles"; }

    void mergeInputRoles();
    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

class FileRulesMerger : public SectionMerger
{
public:
    FileRulesMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        ConfigMergingParameters::MergeStrategies strat = options.params->getFileRules();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }
    }

private:
    const std::string getName() const { return "FileRules"; }

    void addRulesIfNotPresent(const ConstFileRulesRcPtr & input, 
                              FileRulesRcPtr & merged) const;
    void addRulesAndOverwrite(const ConstFileRulesRcPtr & input, FileRulesRcPtr & merged) const;

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

class DisplayViewMerger : public SectionMerger
{
public:
    DisplayViewMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        ConfigMergingParameters::MergeStrategies strat = options.params->getDisplayViews();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }
    }

private:
    const std::string getName() const { return "Display/Views"; }

    void addUniqueDisplays(const ConstConfigRcPtr & cfg);
    void processDisplays(const ConstConfigRcPtr & first,
                        const ConstConfigRcPtr & second,
                        bool preferSecond);

    void addUniqueVirtualViews(const ConstConfigRcPtr & cfg);
    void processVirtualDisplay(const ConstConfigRcPtr & first,
                            const ConstConfigRcPtr & second,
                            bool preferSecond);

    void addUniqueSharedViews(const ConstConfigRcPtr & cfg);
    void processSharedViews(const ConstConfigRcPtr & first,
                            const ConstConfigRcPtr & second,
                            bool preferSecond);

    void processActiveLists();

    void addUniqueViewTransforms(const ConstConfigRcPtr & cfg);
    void processViewTransforms(const ConstConfigRcPtr & first,
                               const ConstConfigRcPtr & second,
                               bool preferSecond);

    void processViewingRules(const ConstConfigRcPtr & first,
                             const ConstConfigRcPtr & second,
                             bool preferSecond) const;

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

class LooksMerger : public SectionMerger
{
public:
    LooksMerger(MergeHandlerOptions options) : SectionMerger(options)
    {    
        ConfigMergingParameters::MergeStrategies strat = options.params->getLooks();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }
    }

private:
    const std::string getName() const { return "Looks"; }

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

class ColorspacesMerger : public SectionMerger
{
public:
    ColorspacesMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        ConfigMergingParameters::MergeStrategies strat = options.params->getColorspaces();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }

        m_state = MERGERSTATE_NOFLAG;
    }

    struct ColorspaceNameConflict
    {
        bool csInColorspaces = false;
        bool csInNameTransforms = false;
        bool csInAliases = false;

        // Alias to alias conflict
        bool aliasInAliaes = false;
    };

private:
    // Attributes
    MergerFlag m_state;

    std::vector<std::string> m_colorspaceMarkedToBeDeleted;
    std::vector<std::string> m_colorspaceReplacingMarkedCs;

    // Methods
    const std::string getName() const { return "Colorspaces"; }

    void processSearchPaths() const;

    void iterateOverColorSpaces(ConstConfigRcPtr cfg,
                                std::function<void(ColorSpaceRcPtr & colorspace)> cb) const;

    void detectSpecificIndirectConflicts(const char * name, ColorspaceNameConflict & nc) const;

    void nameConflictWithColorspaceName(const ConstColorSpaceRcPtr & srcCs,
                                                  ConfigRcPtr & dstCfg) const;

    void nameConflictWithNameTransformsName(const ConstColorSpaceRcPtr & srcCs,
                                            ConfigRcPtr & dstCfg) const;

    void nameConflictWithColorspaceAliases(const ConstColorSpaceRcPtr & srcCs,
                                              ConfigRcPtr & dstCfg) const;
    void aliasConflictWithColorspaceAliases(const ConstColorSpaceRcPtr & srcCs,
                                            ConfigRcPtr & dstCfg) const;

    void resolveConflict(const ConstConfigRcPtr & source,
                         ConfigRcPtr & destination) const;

    void resolveConflict(ConfigRcPtr & first,
                         ConfigRcPtr & second) const;


    void attemptToAddAlias(const ConstConfigRcPtr & mergeConfig,
                                              ColorSpaceRcPtr & dupeCS,
                                              const ConstColorSpaceRcPtr & inputCS,
                                              const char * aliasName);

//    void updateFamily(ColorSpaceRcPtr & incomingCs, bool fromBase) const;
    void updateFamily(std::string & family, bool fromBase) const;
    bool colorSpaceMayBeMerged(const ConstConfigRcPtr & mergeConfig, 
                               const ConstColorSpaceRcPtr & inputCS) const;
    void mergeColorSpace(ConfigRcPtr & mergeConfig, 
                         ColorSpaceRcPtr & eInputCS,
                         std::vector<std::string> & addedInputColorSpaces);
    void initializeRefSpaceConverters(ConstTransformRcPtr & inputToBaseGtScene,
                                      ConstTransformRcPtr & inputToBaseGtDisplay);
    void addColorSpaces();



    bool handleAddCsErrorNameIdenticalToARoleName(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorNameIdenticalToNTNameOrAlias(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorNameContainCtxVarToken(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorNameIdenticalToExistingColorspaceAlias(ColorSpaceRcPtr & incomingCs) const;;
    bool handleAddCsErrorAliasIdenticalToARoleName(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorAliasIdenticalToNTNameOrAlias(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorAliasContainCtxVarToken(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorAliasIdenticalToExistingColorspaceName(ColorSpaceRcPtr & incomingCs) const;
    bool handleAddCsErrorAliasIdenticalToExistingColorspaceAlias(ColorSpaceRcPtr & incomingCs) const;
    
    void handleErrorCodeWhenAddingInputFirst(ColorSpaceRcPtr & eColorspace);
    void handleErrorCodes(ColorSpaceRcPtr & eColorspace);

    bool handleAvoidDuplicatesOption(ConfigRcPtr & eBase, ColorSpaceRcPtr & eColorspace);
    void handleAssumeCommonReferenceOption(ColorSpaceRcPtr & eColorspace);

    void handleMarkedToBeDeletedColorspaces();
    bool isColorspaceMarked(const ConstColorSpaceRcPtr & cs);

    void addFamilyPrefix(ColorSpaceRcPtr & incomingCs, std::string prefix) const;
    void replaceFamilySeparatorInFamily(ColorSpaceRcPtr & incomingCs);

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

class NamedTransformsMerger : public SectionMerger
{
public:
    NamedTransformsMerger(MergeHandlerOptions options) : SectionMerger(options)
    {
        ConfigMergingParameters::MergeStrategies strat = options.params->getNamedTransforms();
        if (strat != ConfigMergingParameters::MergeStrategies::STRATEGY_UNSET)
        {
            m_strategy = strat;
        }
        else
        {
            m_strategy = options.params->getDefaultStrategy();
        }

        m_state = MERGERSTATE_NOFLAG;
    }

private:
    // Attributes
    MergerFlag m_state;

    // Methods
    const std::string getName() const { return "Named Transforms"; }

    void updateFamily(std::string & family, bool fromBase) const;
    bool namedTransformMayBeMerged(const ConstConfigRcPtr & mergeConfig, 
                                   const ConstNamedTransformRcPtr & nt,
                                   bool fromBase) const;
    void mergeNamedTransform(ConfigRcPtr & mergeConfig, 
                             NamedTransformRcPtr & eNT,
                             bool fromBase,
                             std::vector<std::string> & addedInputNamedTransforms);
    void addNamedTransforms();

    void iterateOverColorSpaces(ConstConfigRcPtr cfg, std::function<void(ColorSpaceRcPtr &)> cb) const;

    bool handleAddNtErrorNeedAtLeastOneTransform(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorNameIdenticalToARoleName(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorNameIdenticalToColorspaceNameOrAlias(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorNameContainCtxVarToken(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorNameIdenticalToExistingNtAlias(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorAliasIdenticalToARoleName(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorAliasIdenticalToColorspaceNameOrAlias(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorAliasContainCtxVarToken(NamedTransformRcPtr & incomingNt) const;
    bool handleAddNtErrorAliasIdenticalToExistingNtAlias(NamedTransformRcPtr & incomingNt) const;
    
    void handleErrorCodeWhenAddingInputFirst(NamedTransformRcPtr & eNamedTransform);
    void handleErrorCodes(NamedTransformRcPtr & eColorspace);

    void handlePreferInput();
    void handlePreferBase();    
    void handleInputOnly();
    void handleBaseOnly();
    void handleRemove();
};

}  // namespace OCIO_NAMESPACE

#endif // INCLUDED_OCIO_MERGE_CONFIG_SECTION_MERGER_H
