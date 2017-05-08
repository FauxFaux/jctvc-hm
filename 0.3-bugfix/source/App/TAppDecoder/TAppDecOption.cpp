/*
 * TAppOption 1.3
 *
 * kishan at hackorama dot com  www.hackorama.com JULY 2001
 *
 * + Acts as a common facade class for reading
 *   commandline options as well as options from
 *   an optionfile with delimited type value pairs
 *
 * + Handles the POSIX style single character options ( -w )
 *   as well as the newer GNU long options ( --width )
 *
 * + The option file assumes the traditional format of
 *   first character based comment lines and type value
 *   pairs with a delimiter , and flags which are not pairs
 *
 *    # this is a coment
 *    # next line is an option value pair
 *    width : 100
 *      # next line is a flag
 *      noimages
 *
 * + Supports printing out Help and Usage
 *
 * + Why not just use getopt() ?
 *
 *   getopt() Its a POSIX standard not part of ANSI-C.
 *   So it may not be available on platforms like Windows.
 *
 * + Why it is so long ?
 *
 *   The actual code which does command line parsing
 *   and option file parsing are done in  few methods.
 *   Most of the extra code are for providing a flexible
 *   common public interface to both a resourcefile and
 *   and command line supporting POSIX style and
 *   GNU long option as well as mixing of both.
 *
 * + Please see "anyoption.h" for public method descriptions
 *
 */

/* Updated Auguest 2004
 * Fix from  Michael D Peters (mpeters at sandia.gov)
 * to remove static local variables, allowing multiple instantiations
 * of the reader (for using multiple configuration files).  There is
 * an error in the destructor when using multiple instances, so you
 * cannot delete your objects (it will crash), but not calling the
 * destructor only introduces a small memory leak, so I
 * have not bothered tracking it down.
 *
 * Also updated to use modern C++ style headers, rather than
 * depricated iostream.h (it was causing my compiler problems)
*/

/*
 * Updated September 2006
 * Fix from Boyan Asenov for a bug in mixing up option indexes
 * leading to exception when mixing different options types
 */

/** \file     TAppDecOption.cpp
    \brief    kishan's text parsing library
*/

#include <cstring>
#include "TAppDecOption.h"

TAppOption::TAppOption()
{
  init();
}

TAppOption::TAppOption(int maxopt)
{
  init( maxopt , maxopt );
}

TAppOption::TAppOption(int maxopt, int maxcharopt)
{
  init( maxopt , maxcharopt );
}

TAppOption::~TAppOption()
{
  if( mem_allocated )
    cleanup();
}

void
TAppOption::init()
{
  init( DEFAULT_MAXOPTS , DEFAULT_MAXOPTS );
}

void
TAppOption::init(int maxopt, int maxcharopt )
{

  max_options   = maxopt;
  max_char_options = maxcharopt;
  max_usage_lines = DEFAULT_MAXUSAGE;
  usage_lines = 0 ;
  argc    = 0;
  argv    = NULL;
  posix_style = true;
  verbose   = false;
  filename  = NULL;
  appname   = NULL;
  option_counter  = 0;
  optchar_counter = 0;
  new_argv  = NULL;
  new_argc  = 0 ;
  max_legal_args  = 0 ;
  command_set   = false;
  file_set  = false;
  values    = NULL;
  g_value_counter = 0;
  mem_allocated   = false;
  command_set   = false;
  file_set  = false;
  opt_prefix_char     = '-';
  file_delimiter_char = ':';
  file_comment_char   = '#';
  equalsign   = '=';
  comment       = '#' ;
  delimiter     = ':' ;
  endofline     = '\n';
  whitespace    = ' ' ;
  nullterminate = '\0';
  set = false;
  once = true;
  hasoptions = false;
  autousage = false;

  strcpy( long_opt_prefix , "--" );

  if( alloc() == false ){
    cout << endl << "OPTIONS ERROR : Failed allocating memory" ;
    cout << endl ;
    cout << "Exiting." << endl ;
    exit (0);
  }
}

bool
TAppOption::alloc()
{
  int i = 0 ;
  int size = 0 ;

  if( mem_allocated )
    return true;

  size = (max_options+1) * sizeof(const char*);
  options = (const char**)malloc( size );
  optiontype = (int*) malloc( (max_options+1)*sizeof(int) );
  optionindex = (int*) malloc( (max_options+1)*sizeof(int) );
  if( options == NULL || optiontype == NULL || optionindex == NULL )
    return false;
  else
    mem_allocated  = true;
  for( i = 0 ; i < max_options ; i++ ){
    options[i] = NULL;
    optiontype[i] = 0 ;
    optionindex[i] = -1 ;
  }
  optionchars = (char*) malloc( (max_char_options+1)*sizeof(char) );
  optchartype = (int*) malloc( (max_char_options+1)*sizeof(int) );
  optcharindex = (int*) malloc( (max_char_options+1)*sizeof(int) );
  if( optionchars == NULL ||
            optchartype == NULL ||
            optcharindex == NULL )
        {
    mem_allocated = false;
    return false;
  }
  for( i = 0 ; i < max_char_options ; i++ ){
    optionchars[i] = '0';
    optchartype[i] = 0 ;
    optcharindex[i] = -1 ;
  }

  size = (max_usage_lines+1) * sizeof(const char*) ;
  usage = (const char**) malloc( size );

  if( usage == NULL  ){
    mem_allocated = false;
    return false;
  }
  for( i = 0 ; i < max_usage_lines ; i++ )
    usage[i] = NULL;

  return true;
}

bool
TAppOption::doubleOptStorage()
{
  options = (const char**)realloc( options,
      ((2*max_options)+1) * sizeof( const char*) );
  optiontype = (int*) realloc(  optiontype ,
      ((2 * max_options)+1)* sizeof(int) );
  optionindex = (int*) realloc(  optionindex,
      ((2 * max_options)+1) * sizeof(int) );
  if( options == NULL || optiontype == NULL || optionindex == NULL )
    return false;
  /* init new storage */
  for( int i = max_options ; i < 2*max_options ; i++ ){
    options[i] = NULL;
    optiontype[i] = 0 ;
    optionindex[i] = -1 ;
  }
  max_options = 2 * max_options ;
  return true;
}

bool
TAppOption::doubleCharStorage()
{
  optionchars = (char*) realloc( optionchars,
      ((2*max_char_options)+1)*sizeof(char) );
  optchartype = (int*) realloc( optchartype,
      ((2*max_char_options)+1)*sizeof(int) );
  optcharindex = (int*) realloc( optcharindex,
      ((2*max_char_options)+1)*sizeof(int) );
  if( optionchars == NULL ||
      optchartype == NULL ||
      optcharindex == NULL )
    return false;
  /* init new storage */
  for( int i = max_char_options ; i < 2*max_char_options ; i++ ){
    optionchars[i] = '0';
    optchartype[i] = 0 ;
    optcharindex[i] = -1 ;
  }
  max_char_options = 2 * max_char_options;
  return true;
}

bool
TAppOption::doubleUsageStorage()
{
  usage = (const char**)realloc( usage,
      ((2*max_usage_lines)+1) * sizeof( const char*) );
  if ( usage == NULL )
    return false;
  for( int i = max_usage_lines ; i < 2*max_usage_lines ; i++ )
    usage[i] = NULL;
  max_usage_lines = 2 * max_usage_lines ;
  return true;

}


void
TAppOption::cleanup()
{
  free (options);
  free (optiontype);
  free (optionindex);
  free (optionchars);
  free (optchartype);
  free (optcharindex);
  free (usage);

  for (int i = 0; i < g_value_counter; i++)
  {
    if( values[i] != NULL )
      free(values[i]);
  }
  if( values != NULL )
    free (values);

  if( new_argv != NULL )
    free (new_argv);
}

void
TAppOption::setCommandPrefixChar( char _prefix )
{
  opt_prefix_char = _prefix;
}

void
TAppOption::setCommandLongPrefix( char *_prefix )
{
  if( strlen( _prefix ) > MAX_LONG_PREFIX_LENGTH ){
    *( _prefix + MAX_LONG_PREFIX_LENGTH ) = '\0';
  }

  strcpy (long_opt_prefix,  _prefix);
}

void
TAppOption::setFileCommentChar( char _comment )
{
  file_delimiter_char = _comment;
}


void
TAppOption::setFileDelimiterChar( char _delimiter )
{
  file_comment_char = _delimiter ;
}

bool
TAppOption::CommandSet()
{
  return( command_set );
}

bool
TAppOption::FileSet()
{
  return( file_set );
}

void
TAppOption::noPOSIX()
{
  posix_style = false;
}

bool
TAppOption::POSIX()
{
  return posix_style;
}


void
TAppOption::setVerbose()
{
  verbose = true ;
}

void
TAppOption::printVerbose()
{
  if( verbose )
    cout << endl  ;
}
void
TAppOption::printVerbose( const char *msg )
{
  if( verbose )
    cout << msg  ;
}

void
TAppOption::printVerbose( char *msg )
{
  if( verbose )
    cout << msg  ;
}

void
TAppOption::printVerbose( char ch )
{
  if( verbose )
    cout << ch ;
}

bool
TAppOption::hasOptions()
{
  return hasoptions;
}

void
TAppOption::autoUsagePrint(bool _autousage)
{
  autousage = _autousage;
}

void
TAppOption::useCommandArgs( int _argc, char **_argv )
{
  argc = _argc;
  argv = _argv;
  command_set = true;
  appname = argv[0];
  if(argc > 1) hasoptions = true;
}

void
TAppOption::useFiileName( const char *fname )
{
  filename = fname;
  file_set = true;
}

/*
 * set methods for options
 */

void
TAppOption::setCommandOption( const char *opt )
{
  addOption( opt , COMMAND_OPT );
  g_value_counter++;
}

void
TAppOption::setCommandOption( char opt )
{
  addOption( opt , COMMAND_OPT );
  g_value_counter++;
}

void
TAppOption::setCommandOption( const char *opt , char optchar )
{
  addOption( opt , COMMAND_OPT );
  addOption( optchar , COMMAND_OPT );
  g_value_counter++;
}

void
TAppOption::setCommandFlag( const char *opt )
{
  addOption( opt , COMMAND_FLAG );
  g_value_counter++;
}

void
TAppOption::setCommandFlag( char opt )
{
  addOption( opt , COMMAND_FLAG );
  g_value_counter++;
}

void
TAppOption::setCommandFlag( const char *opt , char optchar )
{
  addOption( opt , COMMAND_FLAG );
  addOption( optchar , COMMAND_FLAG );
  g_value_counter++;
}

void
TAppOption::setFileOption( const char *opt )
{
  addOption( opt , FILE_OPT );
  g_value_counter++;
}

void
TAppOption::setFileOption( char opt )
{
  addOption( opt , FILE_OPT );
  g_value_counter++;
}

void
TAppOption::setFileOption( const char *opt , char optchar )
{
  addOption( opt , FILE_OPT );
  addOption( optchar, FILE_OPT  );
  g_value_counter++;
}

void
TAppOption::setFileFlag( const char *opt )
{
  addOption( opt , FILE_FLAG );
  g_value_counter++;
}

void
TAppOption::setFileFlag( char opt )
{
  addOption( opt , FILE_FLAG );
  g_value_counter++;
}

void
TAppOption::setFileFlag( const char *opt , char optchar )
{
  addOption( opt , FILE_FLAG );
  addOption( optchar , FILE_FLAG );
  g_value_counter++;
}

void
TAppOption::setOption( const char *opt )
{
  addOption( opt , COMMON_OPT );
  g_value_counter++;
}

void
TAppOption::setOption( char opt )
{
  addOption( opt , COMMON_OPT );
  g_value_counter++;
}

void
TAppOption::setOption( const char *opt , char optchar )
{
  addOption( opt , COMMON_OPT );
  addOption( optchar , COMMON_OPT );
  g_value_counter++;
}

void
TAppOption::setFlag( const char *opt )
{
  addOption( opt , COMMON_FLAG );
  g_value_counter++;
}

void
TAppOption::setFlag( const char opt )
{
  addOption( opt , COMMON_FLAG );
  g_value_counter++;
}

void
TAppOption::setFlag( const char *opt , char optchar )
{
  addOption( opt , COMMON_FLAG );
  addOption( optchar , COMMON_FLAG );
  g_value_counter++;
}

void
TAppOption::addOption( const char *opt, int type )
{
  if( option_counter >= max_options ){
    if( doubleOptStorage() == false ){
      addOptionError( opt );
      return;
    }
  }
  options[ option_counter ] = opt ;
  optiontype[ option_counter ] =  type ;
  optionindex[ option_counter ] = g_value_counter;
  option_counter++;
}

void
TAppOption::addOption( char opt, int type )
{
  if( !POSIX() ){
    printVerbose("Ignoring the option character \"");
    printVerbose(  opt );
    printVerbose( "\" ( POSIX options are turned off )" );
    printVerbose();
    return;
  }


  if( optchar_counter >= max_char_options ){
    if( doubleCharStorage() == false ){
      addOptionError( opt );
      return;
    }
  }
  optionchars[ optchar_counter ] =  opt ;
  optchartype[ optchar_counter ] =  type ;
  optcharindex[ optchar_counter ] = g_value_counter;
  optchar_counter++;
}

void
TAppOption::addOptionError( const char *opt )
{
  cout << endl ;
  cout << "OPTIONS ERROR : Failed allocating extra memory " << endl ;
  cout << "While adding the option : \""<< opt << "\"" << endl;
  cout << "Exiting." << endl ;
  cout << endl ;
  exit(0);
}

void
TAppOption::addOptionError( char opt )
{
  cout << endl ;
  cout << "OPTIONS ERROR : Failed allocating extra memory " << endl ;
  cout << "While adding the option: \""<< opt << "\"" << endl;
  cout << "Exiting." << endl ;
  cout << endl ;
  exit(0);
}

void
TAppOption::processOptions()
{
  if( ! valueStoreOK() )
    return;
}

void
TAppOption::processCommandArgs(int max_args)
{
  max_legal_args = max_args;
  processCommandArgs();
}

void
TAppOption::processCommandArgs( int _argc, char **_argv, int max_args )
{
  max_legal_args = max_args;
  processCommandArgs(  _argc, _argv );
}

void
TAppOption::processCommandArgs( int _argc, char **_argv )
{
  useCommandArgs( _argc, _argv );
  processCommandArgs();
}

void
TAppOption::processCommandArgs()
{
    if( ! ( valueStoreOK() && CommandSet() )  )
     return;

  if( max_legal_args == 0 )
    max_legal_args = argc;
  new_argv = (int*) malloc( (max_legal_args+1) * sizeof(int) );
  for( int i = 1 ; i < argc ; i++ ){/* ignore first argv */
    if(  argv[i][0] == long_opt_prefix[0] &&
                     argv[i][1] == long_opt_prefix[1] ) { /* long GNU option */
      int match_at = parseGNU( argv[i]+2 ); /* skip -- */
      if( match_at >= 0 && i < argc-1 ) /* found match */
        setValue( options[match_at] , argv[++i] );
    }else if(  argv[i][0] ==  opt_prefix_char ) { /* POSIX char */
      if( POSIX() ){
        char ch =  parsePOSIX( argv[i]+1 );/* skip - */
        if( ch != '0' && i < argc-1 ) /* matching char */
          setValue( ch ,  argv[++i] );
      } else { /* treat it as GNU option with a - */
        int match_at = parseGNU( argv[i]+1 ); /* skip - */
        if( match_at >= 0 && i < argc-1 ) /* found match */
          setValue( options[match_at] , argv[++i] );
      }
    }else { /* not option but an argument keep index */
      if( new_argc < max_legal_args ){
                                new_argv[ new_argc ] = i ;
                                new_argc++;
                        }else{ /* ignore extra arguments */
                                printVerbose( "Ignoring extra argument: " );
        printVerbose( argv[i] );
        printVerbose( );
        printAutoUsage();
                        }
    }
  }
}

char
TAppOption::parsePOSIX( char* arg )
{

  for( unsigned int i = 0 ; i < strlen(arg) ; i++ ){
    char ch = arg[i] ;
    if( matchChar(ch) ) { /* keep matching flags till an option */
      /*if last char argv[++i] is the value */
      if( i == strlen(arg)-1 ){
        return ch;
      }else{/* else the rest of arg is the value */
        i++; /* skip any '=' and ' ' */
        while( arg[i] == whitespace
              || arg[i] == equalsign )
          i++;
        setValue( ch , arg+i );
        return '0';
      }
    }
  }
  return '0';
}

int
TAppOption::parseGNU( char *arg )
{
  int split_at = 0;
  /* if has a '=' sign get value */
  for( unsigned int i = 0 ; i < strlen(arg) ; i++ ){
    if(arg[i] ==  equalsign ){
      split_at = i ; /* store index */
      i = (unsigned int)strlen(arg); /* get out of loop */
    }
  }
  if( split_at > 0 ){ /* it is an option value pair */
    char* tmp = (char*) malloc(  (split_at+1)*sizeof(char) );
    for( int i = 0 ; i < split_at ; i++ )
      tmp[i] = arg[i];
    tmp[split_at] = '\0';

    if ( matchOpt( tmp ) >= 0 ){
      setValue( options[matchOpt(tmp)] , arg+split_at+1 );
      free (tmp);
    }else{
      free (tmp);
      return -1;
    }
  }else{ /* regular options with no '=' sign  */
    return  matchOpt(arg);
  }
  return -1;
}


int
TAppOption::matchOpt( char *opt )
{
  for( int i = 0 ; i < option_counter ; i++ ){
    if( strcmp( options[i], opt ) == 0 ){
      if( optiontype[i] ==  COMMON_OPT ||
          optiontype[i] ==  COMMAND_OPT )
      { /* found option return index */
        return i;
      }else if( optiontype[i] == COMMON_FLAG ||
             optiontype[i] == COMMAND_FLAG )
      { /* found flag, set it */
        setFlagOn( opt );
        return -1;
      }
    }
  }
  return  -1;
}
bool
TAppOption::matchChar( char c )
{
  for( int i = 0 ; i < optchar_counter ; i++ ){
    if( optionchars[i] == c ) { /* found match */
      if(optchartype[i] == COMMON_OPT ||
           optchartype[i] == COMMAND_OPT )
      { /* an option store and stop scanning */
        return true;
      }else if( optchartype[i] == COMMON_FLAG ||
          optchartype[i] == COMMAND_FLAG ) { /* a flag store and keep scanning */
        setFlagOn( c );
        return false;
      }
    }
  }
  return false;
}

bool
TAppOption::valueStoreOK( )
{
  int size= 0;
  if( !set ){
    if( g_value_counter > 0 ){
      size = g_value_counter * sizeof(char*);
      values = (char**)malloc( size );
      for( int i = 0 ; i < g_value_counter ; i++)
        values[i] = NULL;
      set = true;
    }
  }
  return  set;
}

/*
 * public get methods
 */
char*
TAppOption::getValue( const char *option )
{
  if( !valueStoreOK() )
    return NULL;

  for( int i = 0 ; i < option_counter ; i++ ){
    if( strcmp( options[i], option ) == 0 )
      return values[ optionindex[i] ];
  }
  return NULL;
}

bool
TAppOption::getFlag( const char *option )
{
  if( !valueStoreOK() )
    return false;
  for( int i = 0 ; i < option_counter ; i++ ){
    if( strcmp( options[i], option ) == 0 )
      return findFlag( values[ optionindex[i] ] );
  }
  return false;
}

char*
TAppOption::getValue( char option )
{
  if( !valueStoreOK() )
    return NULL;
  for( int i = 0 ; i < optchar_counter ; i++ ){
    if( optionchars[i] == option )
      return values[ optcharindex[i] ];
  }
  return NULL;
}

bool
TAppOption::getFlag( char option )
{
  if( !valueStoreOK() )
    return false;
  for( int i = 0 ; i < optchar_counter ; i++ ){
    if( optionchars[i] == option )
      return findFlag( values[ optcharindex[i] ] ) ;
  }
  return false;
}

bool
TAppOption::findFlag( char* val )
{
  if( val == NULL )
    return false;

  if( strcmp( TRUE_FLAG , val ) == 0 )
    return true;

  return false;
}

/*
 * private set methods
 */
bool
TAppOption::setValue( const char *option , char *value )
{
  if( !valueStoreOK() )
    return false;
        for( int i = 0 ; i < option_counter ; i++ ){
                if( strcmp( options[i], option ) == 0 ){
                        values[ optionindex[i] ] = (char*) malloc((strlen(value)+1)*sizeof(char));
                        strcpy( values[ optionindex[i] ], value );
      return true;
    }
        }
        return false;
}

bool
TAppOption::setFlagOn( const char *option )
{
  if( !valueStoreOK() )
    return false;
        for( int i = 0 ; i < option_counter ; i++ ){
                if( strcmp( options[i], option ) == 0 ){
                        values[ optionindex[i] ] = (char*) malloc((strlen(TRUE_FLAG)+1)*sizeof(char));
                        strcpy( values[ optionindex[i] ]  ,  TRUE_FLAG );
      return true;
    }
        }
        return false;
}

bool
TAppOption::setValue( char option , char *value )
{
  if( !valueStoreOK() )
    return false;
        for( int i = 0 ; i < optchar_counter ; i++ ){
                if( optionchars[i] == option ){
                        values[ optcharindex[i] ] = (char*) malloc((strlen(value)+1)*sizeof(char));
                        strcpy( values[ optcharindex[i] ],  value );
      return true;
    }
        }
        return false;
}

bool
TAppOption::setFlagOn( char option )
{
  if( !valueStoreOK() )
    return false;
        for( int i = 0 ; i < optchar_counter ; i++ ){
                if( optionchars[i] == option ){
                        values[ optcharindex[i] ] = (char*) malloc((strlen(TRUE_FLAG)+1)*sizeof(char));
      strcpy( values[ optcharindex[i] ] , TRUE_FLAG );
      return true;
    }
        }
        return false;
}


int
TAppOption::getArgc( )
{
  return new_argc;
}

char*
TAppOption::getArgv( int index )
{
  if( index < new_argc ){
    return ( argv[ new_argv[ index ] ] );
  }
  return NULL;
}

/* dotfile sub routines */

bool
TAppOption::processFile()
{
  if( ! (valueStoreOK() && FileSet())  )
    return false;
        char* pbuff = readFile();
  return  ( consumeFile(pbuff) );
}

bool
TAppOption::processFile( const char *fname )
{
  useFiileName(fname );
  return ( processFile() );
}

char*
TAppOption::readFile()
{
  return ( readFile(filename) );
}

/*
 * read the file contents to a character buffer
 */

char*
TAppOption::readFile( const char* fname )
{
  int length;
  char *buffer;
  ifstream is;
  is.open ( fname , ifstream::in );
  if( ! is.good() )
  {
    is.close();
    printf( "failed to open [%s] CFG file\n", fname );
    return NULL;
  }
  is.seekg (0, ios::end);
  length = int( is.tellg() );
  is.seekg (0, ios::beg);
//        buffer = (char*) malloc(length*sizeof(char));
  buffer = new char[length]; //kolya
  is.read (buffer, length);
  is.close();
  return buffer;
}

/*
 * scans a char* buffer for lines that does not
 * start with the specified comment character.
 */
bool
TAppOption::consumeFile( char*& buffer )
{
  if( buffer == NULL )
    return false;

  char *cursor = buffer;/* preserve the ptr */
  char *pline = NULL ;
  int linelength = 0;
  bool newline = true;
  for( unsigned int i = 0 ; i < strlen( buffer ) ; i++ ){
    if( *cursor == endofline )/* end of line */
    {
      if( pline != NULL ) /* valid line */
        processLine( pline, linelength );
      pline = NULL;
      newline = true;
    }
    else if( newline ) /* start of line */
    {
      newline = false;
      if( (*cursor != comment ) ) /* not a comment */
      {
        pline = cursor ;
        linelength = 0 ;
      }
    }
    cursor++; /* keep moving */
    linelength++;
  }
  delete [] buffer; //kolya
  buffer = NULL;
  return true;
}


/*
 *  find a valid type value pair separated by a delimiter
 *  character and pass it to valuePairs()
 *  any line which is not valid will be considered a value
 *  and will get passed on to justValue()
 *
 *  assuming delimiter is ':' the behaviour will be,
 *
 *  width:10    - valid pair valuePairs( width, 10 );
 *  width : 10  - valid pair valuepairs( width, 10 );
 *
 *  ::::        - not valid
 *  width       - not valid
 *  :10         - not valid
 *  width:      - not valid
 *  ::          - not valid
 *  :           - not valid
 *
 */

void
TAppOption::processLine( char *theline, int length  )
{
  bool found = false;
  char *pline = new char[length+1];
  for( int i = 0 ; i < length ; i ++ )
    pline[i]= *(theline++);
  pline[length] = nullterminate;
  char *cursor = pline ; /* preserve the ptr */
  if( *cursor == delimiter || *(cursor+length-1) == delimiter )
  {
    justValue( pline );/* line with start/end delimiter */
  }
  else
  {
    for( int i = 1 ; i < length-1 && !found ; i++){/* delimiter */
      if( *cursor == delimiter )
      {
        *(cursor-1) = nullterminate; /* two strings */
        found = true;
        valuePairs( pline , cursor+1 );
      }
      cursor++;
    }
    cursor++;
    if( !found ) /* not a pair */
      justValue( pline );
  }
  delete [] pline;
  pline = NULL;
}

/*
 * removes trailing and preceeding whitespaces from a string
 */
char*
TAppOption::chomp( char *str )
{
        while( *str == whitespace )
                str++;
        char *end = str+strlen(str)-1;
        while( *end == whitespace )
                end--;
        *(end+1) = nullterminate;
        return str;
}

void
TAppOption::valuePairs( char *type, char *value )
{
  if ( strlen(chomp(type)) == 1  ){ /* this is a char option */
    for( int i = 0 ; i < optchar_counter ; i++ ){
      if(  optionchars[i] == type[0]  ){ /* match */
        if( optchartype[i] == COMMON_OPT ||
            optchartype[i] == FILE_OPT )
        {
          setValue( type[0] , chomp(value) );
          return;
        }
      }
    }
  }
  /* if no char options matched */
  for( int i = 0 ; i < option_counter ; i++ ){
    if( strcmp( options[i], type ) == 0 ){ /* match */
      if( optiontype[i] == COMMON_OPT ||
          optiontype[i] == FILE_OPT )
      {
        setValue( type , chomp(value) );
        return;
      }
    }
  }
        printVerbose( "Unknown option in resourcefile : " );
  printVerbose( type );
  printVerbose( );
}

void
TAppOption::justValue( char *type )
{

  if ( strlen(chomp(type)) == 1  ){ /* this is a char option */
    for( int i = 0 ; i < optchar_counter ; i++ ){
      if(  optionchars[i] == type[0]  ){ /* match */
        if( optchartype[i] == COMMON_FLAG ||
            optchartype[i] == FILE_FLAG )
        {
          setFlagOn( type[0] );
          return;
        }
      }
    }
  }
  /* if no char options matched */
  for( int i = 0 ; i < option_counter ; i++ ){
    if( strcmp( options[i], type ) == 0 ){ /* match */
      if( optiontype[i] == COMMON_FLAG ||
          optiontype[i] == FILE_FLAG )
      {
        setFlagOn( type );
        return;
      }
    }
  }
        printVerbose( "Unknown option in resourcefile : " );
  printVerbose( type  );
  printVerbose( );
}

/*
 * usage and help
 */


void
TAppOption::printAutoUsage()
{
  if( autousage ) printUsage();
}

void
TAppOption::printUsage()
{

  if( once ) {
    once = false ;
    cout << endl ;
    for( int i = 0 ; i < usage_lines ; i++ )
      cout << usage[i] << endl ;
    cout << endl ;
  }
}


void
TAppOption::addUsage( const char *line )
{
  if( usage_lines >= max_usage_lines ){
    if( doubleUsageStorage() == false ){
      addUsageError( line );
      exit(1);
    }
  }
  usage[ usage_lines ] = line ;
  usage_lines++;
}

void
TAppOption::addUsageError( const char *line )
{
  cout << endl ;
  cout << "OPTIONS ERROR : Failed allocating extra memory " << endl ;
  cout << "While adding the usage/help  : \""<< line << "\"" << endl;
  cout << "Exiting." << endl ;
  cout << endl ;
  exit(0);

}

