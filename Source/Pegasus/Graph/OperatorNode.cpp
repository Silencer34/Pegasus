/****************************************************************************************/
/*                                                                                      */
/*                                       Pegasus                                        */
/*                                                                                      */
/****************************************************************************************/

//! \file	OperatorNode.cpp
//! \author	Kevin Boulanger
//! \date	23rd October 2013
//! \brief	Base operator node class, for all operators with at least one input

#include "Pegasus/Graph/OperatorNode.h"

namespace Pegasus {
namespace Graph {


OperatorNode::OperatorNode(Alloc::IAllocator* nodeAllocator, Alloc::IAllocator* nodeDataAllocator)
:   Node(nodeAllocator, nodeDataAllocator)
{
}

//----------------------------------------------------------------------------------------

bool OperatorNode::Update()
{
    // Check the number of inputs
    const unsigned int minNumInputs = GetMinNumInputNodes();
    const unsigned int maxNumInputs = GetMaxNumInputNodes();
    PG_ASSERTSTR(maxNumInputs >= minNumInputs, "Invalid boundaries for the number of inputs (%d,%d), the max should be >= to the min", minNumInputs, maxNumInputs);
    const unsigned int numInputs = GetNumInputs();
    if ((numInputs < minNumInputs) || (numInputs > maxNumInputs))
    {
        PG_FAILSTR("Invalid number of inputs for a node (%d), it should be between %d and %d",
                   numInputs, minNumInputs, maxNumInputs);
        return IsDataDirty();
    }
    
    // Update every input node and check if any has the dirty flag set
    bool dirtyFlagSet = false;
    for (unsigned i = 0; i < numInputs; ++i)
    {
        dirtyFlagSet |= GetInput(i)->Update();
    }

    if (dirtyFlagSet)
    {
        // If any input is dirty, invalidate the node data if allocated
        InvalidateData();
        return true;
    }
    else
    {
        // If no input is dirty, return the state of the node data
        return IsDataDirty();
    }
}

//----------------------------------------------------------------------------------------
    
NodeDataReturn OperatorNode::GetUpdatedData(bool & updated)
{
    // Check the number of inputs
    const unsigned int minNumInputs = GetMinNumInputNodes();
    const unsigned int maxNumInputs = GetMaxNumInputNodes();
    PG_ASSERTSTR(maxNumInputs >= minNumInputs, "Invalid boundaries for the number of inputs (%d,%d), the max should be >= to the min", minNumInputs, maxNumInputs);
    const unsigned int numInputs = GetNumInputs();
    if ((numInputs < minNumInputs) || (numInputs > maxNumInputs))
    {
        PG_FAILSTR("Invalid number of inputs for a node (%d), it should be between %d and %d",
                   numInputs, minNumInputs, maxNumInputs);
        return GetData();
    }

    // If the data has not been allocated, allocate it now
    if (!IsDataAllocated())
    {
        CreateData();
    }
    PG_ASSERTSTR(IsDataAllocated(), "Node data has to be allocated when being updated");

    // Get the updated data for every input
    bool inputUpdated = false;
    for (unsigned int i = 0; i < numInputs; ++i)
    {
        (void) GetInput(i)->GetUpdatedData(inputUpdated);
    }

    // If any input has been updated or if the data is dirty, re-generate them
    if (inputUpdated || IsDataDirty())
    {
        GenerateData();
        GetData()->Validate();
        updated = true;
    }
    PG_ASSERTSTR(!IsDataDirty(), "Node data is supposed to be up-to-date at this point");

    return GetData();
}

//----------------------------------------------------------------------------------------
    
OperatorNode::~OperatorNode()
{
}

//----------------------------------------------------------------------------------------

void OperatorNode::AddInput(NodeIn inputNode)
{
    if (GetNumInputs() < GetMaxNumInputNodes())
    {
        Node::AddInput(inputNode);
    }
    else
    {
        PG_FAILSTR("Unable to add an input node since its maximum number has been reached (%d)", GetMaxNumInputNodes());
    }
}

//----------------------------------------------------------------------------------------

void OperatorNode::ReplaceInput(unsigned int index, const Pegasus::Core::Ref<Node> & inputNode)
{
    // Nothing special to test, the number of inputs does not change
    Node::ReplaceInput(index, inputNode);
}

//----------------------------------------------------------------------------------------

void OperatorNode::OnRemoveInput(unsigned int index)
{
}


}   // namespace Graph
}   // namespace Pegasus
