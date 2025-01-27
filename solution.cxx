#include <iostream>
#include <unistd.h>
#include <memory>
#include <fstream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <cmath>
#include <numeric>

// ================================================================================================================ //

class File {
public:
  File() = delete;
  File( std::string name, std::size_t size )
    : m_name( std::move(name) ), m_size( size ) {}
  ~File() = default;

  const std::string& name() const { return m_name; }
  const std::size_t& size() const { return m_size; }

  void print( const std::string& indent = "" ) const {
    std::cout<<indent << "File '" << m_name <<"' (" << m_size <<  ")"<<std::endl;
  }
  
private:
  std::string m_name;
  std::size_t m_size;
};


class Node {
public:
  Node() = delete;
  Node(std::string name, std::size_t size) :
    m_name( std::move(name) ), m_size( size ),
    m_occupiedMemory( 0 ), m_freeMemory( size ) {}
  ~Node() = default;

  const std::string& name() const { return m_name; }
  const std::size_t& size() const { return m_size; }

  const std::size_t& occupiedMemory() const { return m_occupiedMemory; }
  const std::size_t& freeMemory() const { return m_freeMemory; }
  
  bool canAccept( const File& file ) const { return file.size() <= this->freeMemory(); }
  bool add( const File& file ) {
    m_occupiedMemory += file.size();
    m_freeMemory -= file.size();
    return true;
  }

  void print( const std::string& indent = "") const {
    std::cout<<indent<< "Node '" << m_name << "' (" << this->freeMemory() << "/" << m_size <<  ") [used: " << this->occupiedMemory() << "]"<<std::endl;
  }
  
private:
  std::string m_name;
  std::size_t m_size;
  std::size_t m_occupiedMemory;
  std::size_t m_freeMemory;
};

// ================================================================================================================ //

int Usage();
template< class T > bool processFile(const std::string&,std::vector<T>&);
void allocateNodes(std::vector<std::size_t>&,
                   std::vector<File>&,
                   std::vector<Node>& );
template<typename comparator_t>
std::size_t findNewPositionInRange(std::size_t, std::size_t,
                   const std::vector<std::size_t>&,
                   const comparator_t& Comparator);
void swap(std::vector<std::size_t>&, std::size_t, std::size_t);

// ================================================================================================================ //

int main( int narg, char* argv[] ) {
  std::cout << "Running code ... " << std::endl;
  
  std::string inputFilesName = "";
  std::string inputNodesName = "";
  std::string outputName = "";

  // ================================================================================== //
  
  int c;
  while ( (c = getopt (narg, argv, "f:n:o:h") ) != -1) {
    switch (c) {
    case 'h':
      return Usage();
    case 'f':
      inputFilesName = optarg;
      break;
    case 'n':
      inputNodesName = optarg;
      break;
    case 'o':
      outputName = optarg;
      break;
    case '?':
      if (isprint (optopt))
    fprintf (stderr, "Unknown option `-%c'.\n", optopt);
      else
    fprintf (stderr,"Unknown option character `\\x%x'.\n",optopt);
      return Usage();
    default:
      return Usage();
    }
  }

  
  if ( inputFilesName.empty() ) {
    std::cout<<"### Input missing: file with file names not specified!"<<std::endl;
    return Usage();
  }
  
  if ( inputNodesName.empty() ) {
    std::cout<<"### Input missing: file with nodes not specified!"<<std::endl;
    return Usage();
  }

  // ================================================================================== //

  std::vector< File > listOfFiles;
  std::vector< Node > listOfNodes;
    
  // Read Nodes
  if ( not processFile( inputNodesName,listOfNodes ) )
    return Usage();
  
  // Read Files
  if ( not processFile( inputFilesName,listOfFiles ) )
    return Usage();
  
  // Create output
  std::ofstream output;
  if ( not outputName.empty() ) {
    output.open( outputName.c_str() );
    if ( not output.is_open() ) {
      std::cout<<"ERROR: Cannot open output file: "<<outputName<<std::endl;
      return Usage();
    }
  }
  
  // ================================================================================== //
  
  // Distributing...
  std::vector<std::size_t> distributionPlan;
  distributionPlan.resize(listOfFiles.size(), listOfFiles.size());
  allocateNodes( distributionPlan, listOfFiles, listOfNodes );
  
  // ================================================================================== //
    
  // Writing the output
  for (std::size_t n_file(0); n_file < distributionPlan.size(); n_file++) {
    std::size_t n_node = distributionPlan.at(n_file);
    const auto& file = listOfFiles.at(n_file);

    std::string message = file.name() + " ";
    if (n_node == listOfFiles.size())
      message += "NULL";
    else
      message += listOfNodes.at(n_node).name();
    
    if ( not outputName.empty() ) output << message.c_str() << "\n";
    else std::cout << message.c_str() << "\n";
  }
  
  output.close();
}

// ============================================================ //

int Usage() {

  std::cout<<""<< std::endl;
  std::cout<<"USAGE:  ./solution <OPTIONS>"<<std::endl;
  std::cout<<"  OPTIONS:"<<std::endl;
  std::cout<<"        -h               Print usage information"<<std::endl;
  std::cout<<"        -f <filename>    [REQUIRED] Specify input file with list of file names     "<<std::endl;
  std::cout<<"        -n <filename>    [REQUIRED] Specify input file with list of nodes          "<<std::endl;
  std::cout<<"        -o <filename>    [OPTIONAL] Specify output file (default: standard output) "<<std::endl;
  std::cout<<""<<std::endl;

  return 0;
}

template< class T > bool processFile( const std::string& fileName,
                      std::vector<T>& objectCollection ) {

  std::ifstream inputFiles;
  inputFiles.open( fileName.c_str() );
  if ( not inputFiles.is_open() ) {
    std::cout<<"ERROR: Cannot open input file: "<<fileName<<std::endl;
    return false;
  }

  std::string line = "";
  try {
    while ( std::getline(inputFiles, line) ) {
      // Skip the line if it is a comment
      if ( line.find("#",0) == 0 ) continue;
      
      std::istringstream iss( line );
      std::string name = "";
      std::string size = "";
      std::string otherArgument = "";
      iss >> name >> size >> otherArgument;

      // This happens if the line is empty, or if it contains only white spaces
      // Skip the line in these cases
      if ( name.empty() ) continue;

      // Check if there are additional elements
      // This should not happen
      if ( not otherArgument.empty() ) {
    std::cout<<"ERROR: Too many arguments in the line: something is wrong in the input file '" << fileName << "'" << std::endl;
    std::cout<<"ERROR: Faulty line: " << iss.str() << std::endl;
    inputFiles.close();
        return false;
      }

      // Check the size is a positive integer
      int sizeInt = std::stoi(size);
      if ( sizeInt < 0 ) {
    std::cout<<"ERROR: Size is negative: something is wrong in the input file '" << fileName << "'" << std::endl;
    std::cout<<"ERROR: Faulty line: " << iss.str() << std::endl;
    inputFiles.close();
    return false;
      }

      objectCollection.push_back( T(name, sizeInt) );
    }
  } catch ( ... ) {
    std::cout<<"ERROR: Issues while reading input file: '" << fileName << "'" << std::endl;
    inputFiles.close();
    return false;
  }
  
  inputFiles.close();
  return true;
}

void allocateNodes( std::vector<std::size_t>& distributionPlan,
                    std::vector<File>& listOfFiles,
                    std::vector<Node>& listOfNodes ) {
  
  std::vector<std::size_t> indexes_files(listOfFiles.size());
  std::vector<std::size_t> indexes_nodes(listOfNodes.size());

  std::iota(indexes_files.begin(), indexes_files.end(), 0);
  std::iota(indexes_nodes.begin(), indexes_nodes.end(), 0);

  // Compare functions
  auto compareFiles = [&listOfFiles] (std::size_t elA, std::size_t elB) -> bool
              {
            return listOfFiles[elA].size() > listOfFiles[elB].size();
              };
  
  auto compareNodes = [&listOfNodes] (std::size_t elA, std::size_t elB) -> bool
              {
            const auto& NodeA = listOfNodes[elA];
            const auto& NodeB = listOfNodes[elB];
            if ( NodeA.occupiedMemory() < NodeB.occupiedMemory() ) return true;
            if ( NodeA.occupiedMemory() > NodeB.occupiedMemory() ) return false;
            return NodeA.freeMemory() > NodeB.freeMemory();
              };
  
  // Sort Files in decresing order (size). Big files first.
  std::sort(indexes_files.begin(),
            indexes_files.end(),
            compareFiles );
  
  // Sort Nodes according to node memory. Nodes with big occupied memory last.
  // In case of two nodes with same occupied memory, the node with big free memory goes first.
  std::sort(indexes_nodes.begin(),
            indexes_nodes.end(),
            compareNodes );

  // Running on files
  for ( std::size_t idx_f : indexes_files ) {
    const auto &file = listOfFiles[idx_f];

    // Running on Nodes to allocate the file
    for ( std::size_t j(0); j<indexes_nodes.size(); j++ ) {
      std::size_t idex_n_current = indexes_nodes[j];
      auto& node = listOfNodes[idex_n_current];

      if ( not node.canAccept( file ) ) continue;
      node.add( file );
      distributionPlan[ idx_f ] = idex_n_current;

      // Place the modified Node into the (new) correct position
      std::size_t new_pos = findNewPositionInRange(j, indexes_nodes.size(),
                           indexes_nodes,
                           compareNodes);
      
      swap(indexes_nodes, j, new_pos);
      break;
    }
  }
}

template<typename comparator_t>
std::size_t findNewPositionInRange(std::size_t dw, std::size_t up,
                                   const std::vector<std::size_t>& collection,
                                   const comparator_t& Comparator)
{
  std::size_t output = dw;
  std::size_t mp = 0;
  
  bool found = false;
  while (not found) {
    mp = (output + up)/2;

    if ( Comparator(collection[dw], collection[mp]) ) {
      up = mp;
    } else {
      output = mp;
    }

    if (up - output <= 1)
      found = true;
  }

  return output;
}

void swap(std::vector<std::size_t>& collection,
      std::size_t current, std::size_t target)
{
  if (current == target) return;

  std::size_t storage = collection[current];
  for (std::size_t m(current); m<target; m++)
    collection[m] = collection[m+1];
  collection[target] = storage;
}
