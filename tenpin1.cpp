//*****************************************************************************
/// @file tenpin1.cpp
/// @author Jonathan D. Lettvin
/// Copyright(c)2010 Jonathan D. Lettvin, All Rights Reserved
//*****************************************************************************
/// Score all possible games of tenpin bowling and
/// handle all erroneous and exceptional values.
//*****************************************************************************
/** @brief Typical output
Some tests are pin fall sequences from the URL:
http://en.wikipedia.org/wiki/Ten-pin_bowling#Scoring
with scores matching those documented on wikipedia.
Other tests exercise exception handling.

-------------------------------------------------------------------------
		Normal behavior: 
******* game 1: all gutterballs: *********************************************
   - -   - -   - -   - -   - -   - -   - -   - -   - -   - -   - -
  0     0     0     0     0     0     0     0     0     0        0

******* test 2: one strike and gutterballs (expect score=10): ****************
   X     - -   - -   - -   - -   - -   - -   - -   - -   - -   - -
 10    10    10    10    10    10    10    10    10    10       10 [PASS]

******* test 3: Perfect game (thanksgiving turkey) (expect score=300): *******
   X     X     X     X     X     X     X     X     X     X     X X
 30    60    90   120   150   180   210   240   270   300      300 [PASS]

******* test 4: wikipedia example 1 (strike) (expect score=28): **************
   X     3 6   - -   - -   - -   - -   - -   - -   - -   - -   - -
 19    28    28    28    28    28    28    28    28    28       28 [PASS]

******* test 5: wikipedia example 2 (double) (expect score=57): **************
   X     X     9 -   - -   - -   - -   - -   - -   - -   - -   - -
 29    48    57    57    57    57    57    57    57    57       57 [PASS]

******* test 6: wikipedia example 3 (turkey or triple) (expect score=78): ****
   X     X     X     - 9   - -   - -   - -   - -   - -   - -   - -
 30    50    69    78    78    78    78    78    78    78       78 [PASS]

******* test 7: wikipedia example 4 (expect score=46): ***********************
   X     X     4 2   - -   - -   - -   - -   - -   - -   - -   - -
 24    40    46    46    46    46    46    46    46    46       46 [PASS]

******* test 8: wikipedia example 5 (spare) (expect score=20): ***************
   7 /   4 2   - -   - -   - -   - -   - -   - -   - -   - -   - -
 14    20    20    20    20    20    20    20    20    20       20 [PASS]

******* test 9: final spare (expect score=277): ******************************
   X     X     X     X     X     X     X     X     X     7 /   X  
 30    60    90   120   150   180   210   237   257   277      277 [PASS]

-------------------------------------------------------------------------
		Exception and error handling
******* game 10: too many balls (or frames): *********************************
Catch: 6. too many frames
******* game 11: too few balls: **********************************************
Error: 7. too few balls
******* game 12: too many pins in frame: *************************************
Catch: 2. too many pins for standard frame
******* game 13: >2 balls after final strike: ********************************
Catch: 4. >2 balls after final strike
******* game 14: >1 ball after final spare: **********************************
Catch: 5. >1 ball after final spare
******* game 15: too many pins for ball: *************************************
Catch: 1. too many pins for ball
******* game 16: no pins and no balls: ***************************************
Error: 7. too few balls
******* game 17: excess bonus score: *****************************************
Catch: 1. too many pins for ball
-------------------------------------------------------------------------
 */

#include <iostream>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <string>
#include <vector>
#include <valarray>
#include <map>
#include <algorithm>
#include <exception>

// interface ******************************************************************
namespace nTenPin {

  using namespace std;

  // cPlayer ******************************************************************
  /// @brief cPlayer holds all the scoring and round information for a player.
  /// Document the player instance.
  /// If unit-testing, provide an expected score.
  /// The ctor applies pinfalls to the player scoring.
  /// The display( ) method calculates and outputs the score to a stream
  /// A friend << operator is provided for convenience.
  class cPlayer {
    public:
      cPlayer( const size_t number, const string &doc, const size_t expect=0u );
      ~cPlayer( );
      cPlayer &operator( )( const size_t pins );
      inline cPlayer &operator!( ) { fail_ = true; }
    protected:
      friend ostream &operator<<( ostream &o, cPlayer &player );
      ostream &display( ostream &o );
      const string doc_;			///< Identity of player/test
      size_t number_, total_, round_, ball_, expect_;
      valarray< size_t > score_;		///< Used during display
      vector< valarray< size_t > > pins_;	///< filled by ftor
      bool fail_;				///< To prevent display
  };

  // cUnitTest ****************************************************************
  /// @brief cUnitTest enables a simple instancing of a cPlayer with protection
  /// to allow unit testing to proceed even after exceptions are thrown.
  /// Two operator overloads are used for extreme convenience:
  /// operator= initializes the cPlayer pinfalls with a first value.
  /// operator, adds pinfalls to the cPlayer.
  /// See the void unitTests( ) function for examples.
  class cUnitTest {
    public:
      //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
      cUnitTest( const size_t number, const string &doc, const size_t expect )
	: player_( number, doc, expect ) { }
      //-----------------------------------------------------------------------
      ~cUnitTest( ) { }

      /// operator= overload
      inline cUnitTest &operator=( const size_t fall ) {
	try { player_( fall ); }
	catch( const char * &e ) { commonCatch( cerr, e ); }
	catch( const string &e ) { commonCatch( cerr, e ); }
	return *this;
      }

      /// operator, overload
      inline cUnitTest &operator,( const size_t fall ) {
	try { player_( fall ); }
	catch( const char * &e ) { commonCatch( cerr, e ); }
	catch( const string &e ) { commonCatch( cerr, e ); }
	return *this;
      }
    private:
      /// common catch mechanism
      template< typename T > ostream &commonCatch( ostream &o, const T &e ) {
	!player_;
	o << endl << "Catch: " << e << endl;
	return o;
      }
      cPlayer player_;
  };
}

// implementation *************************************************************
namespace nTenPin {

  using namespace std;

  //+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
  /// ctor allocates storage and initializes values needed to start game.
  cPlayer::cPlayer( const size_t num, const string &doc, const size_t expect )
    :
      doc_(       doc ),
      number_(    num ),
      total_(      0u ),
      round_(      0u ),
      ball_(       0u ),
      expect_( expect ),
      score_(     10u ),
      pins_(       2u ),
      fail_(    false )
  {
    pins_[ 0 ].resize( 11 ); pins_[ 0 ] = 0u;
    pins_[ 1 ].resize( 11 ); pins_[ 1 ] = 0u;
    cout << string( 7, '*' ) << ' ';
    stringstream ss;
    ss << ( expect_ ? "test " : "game " );
    ss << number_ << ": " << doc_;
    if( expect_ ) ss << " (expect score=" << expect_ << ")";
    ss << ": ";
    cout << ss.str( ) << string( 70 - ss.str( ).size( ), '*' );
  }

  //---------------------------------------------------------------------------
  /// dtor shows score when no failure has occurred
  cPlayer::~cPlayer( ) { if( !fail_ ) cout << *this; }

  //()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()()
  /// ftor accepts pinfall counts and stores them into the player structure.
  cPlayer &cPlayer::operator( )( const size_t pins ) {
    if( pins > 10 ) {
      throw( "1. too many pins for ball" );
    } else if( round_ < 10 && ball_ && ( pins_[ 0 ][ round_ ] + pins ) > 10 ) {
      throw( "2. too many pins for standard frame" );
    } else if( ball_ && ( pins_[ 0 ][ round_ ] + pins ) > 20 ) {
      // Superfluous (handled by throw 1)
      throw( "3. too many pins for bonus frame" );
    } else if( round_ == 10 && pins_[ 0 ][ 9 ] == 10 ) {
      /// Handle strike bonus in last frame
      if( ball_ < 2 ) {
	pins_[ ball_ ][ round_ ] = pins;
	++ball_;
      } else {
	throw( "4. >2 balls after final strike" );
      }
    } else if( round_ == 10 && ( pins_[ 0 ][ 9 ] + pins_[ 1 ][ 9 ] ) == 10 ) {
      /// Handle spare bonus in last frame
      if( ball_ < 1 ) {
	pins_[ ball_ ][ round_ ] = pins;
	++ball_;
      } else {
	throw( "5. >1 ball after final spare" );
      }
    } else {
      /// Handle general case (other than last frame)
      if( round_ >= 10 ) {
	throw( "6. too many frames" );
      }
      pins_[ ball_ ][ round_ ] = pins;
      /// Handle strike in first ball of frame
      if( ball_ == 0 && pins == 10 ) ++round_;
      else {
	ball_ = ( ball_ + 1 ) & 1;
	if( !ball_ ) ++round_;
      }
    }
    return *this;
  }

  /// display computes the score and outputs it to the arg stream
  ostream &cPlayer::display( ostream &o ) { //_________________________________
    if( round_ < 10u ) {
      o << endl << "Error: 7. too few balls" << endl;
    } else {
      o << endl;
      total_ = 0;

      /// Individual pinfalls
      for( size_t i = 0; i < 11; ++i ) {
	o << string( 2, ' ' );
	size_t pins0 = pins_[ 0 ][ i ];
	size_t pins1 = pins_[ 1 ][ i ];
	size_t pins  = pins1 + pins0;
	switch( pins0 ) {
	  case 10:
	    /// Handle strikes
	    if( pins1 == 10 ) o << " X X";	///< 2nd strike in bonus frame
	    else o << " X  ";
	    break;
	  case 0:
	    // Handle gutterballs
	    o << " - ";
	    if( pins1 == 0 ) o << '-';
	    else o << pins1;
	    break;
	  default:
	    /// Handle other values
	    o << setw( 2 ) << pins0;
	    if( pins == 10 ) o << " /";		///< Handle spare
	    else if( pins1 == 0 ) o << " -";
	    else o << setw( 2 ) << pins1;
	    break;
	}
      }
      o << endl;

      /// Score summation
      for( size_t i = 0; i < 10; ++i ) {
	/// Pins from this frame.
	size_t pins0 = pins_[ 0 ][ i     ], pins1 = pins_[ 1 ][ i     ];
	/// Pins from the next frame.
	size_t next0 = pins_[ 0 ][ i + 1 ], next1 = pins_[ 1 ][ i + 1 ];
	/// Sum of both frames
	size_t pins  = pins1 + pins0, current = pins;

	if( i == 9 ) {					///< Final frame
	  if( pins0 == 10 ) current += next0 + next1;	///< Strike
	  else if( pins == 10 ) current += next0;	///< Spare
	} else {
	  if( pins0 == 10 ) {				///< Strike
	    if( next0 == 10 ) {				///< Bonus strike
	      current += 10 + pins_[ 0 ][ i + 2 ];
	    }
	    else { current += next0 + next1; }		///< Bonus non-strike
	  } else if( pins == 10 ) { current += next0; }	///< Bonus spare
	}

	total_ += current;
	o << setw( 3 ) << total_ << string( 3, ' ' );
      }

      /// Show unit test pass/fail by comparing total score with expected
      o <<
	string( 3, ' ' ) << setw( 3 ) << total_;
      if( expect_ ) o <<
	" [" << ( ( total_ == expect_ ) ? "PASS" : "FAIL" ) << "]";
      o << endl << endl;
    }
    return o;
  }

  /// friend operator<< simplifies output of a player structure.
  ostream &operator<<( ostream &o, cPlayer &player ) { //<<<<<<<<<<<<<<<<<<<<<<
    player.display( o );
    return o;
  }

  /// unitTests scores various normal and pathological data.
  void unitTests( ) { //ttttttttttttttttttttttttttttttttttttttttttttttttttttttt
    cout <<
      "Some tests are pin fall sequences from the URL:" << endl <<
      "http://en.wikipedia.org/wiki/Ten-pin_bowling#Scoring" << endl <<
      "with scores matching those documented on wikipedia." << endl <<
      "Other tests exercise exception handling." << endl << endl
      ;
    const size_t X = 10u;

    cout << string( 73, '-' ) << endl;
    cout << "\t\tNormal behavior: " << endl;

    cUnitTest( 1, "all gutterballs", 0 ) =
      0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 2, "one strike and gutterballs", 10 ) =
      X,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 3, "Perfect game (thanksgiving turkey)", 300 ) =
      X,X,X,X,X,X,X,X,X,X,X,X;

    cUnitTest( 4, "wikipedia example 1 (strike)", 28 ) =
      X,3,6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 5, "wikipedia example 2 (double)", 57 ) =
      X,X,9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 6, "wikipedia example 3 (turkey or triple)", 78 ) =
      X,X,X,0,9,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 7, "wikipedia example 4", 46 ) =
      X,X,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 8, "wikipedia example 5 (spare)", 20 ) =
      7,3,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;

    cUnitTest( 9, "final spare", 277 ) =
      X,X,X,X,X,X,X,X,X,7,3,X;

    cout << string( 73, '-' ) << endl;
    cout << "\t\tException and error handling" << endl;

    cUnitTest( 10, "too many balls (or frames)", 0 ) =
      7,3,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;	// catch 6

    cUnitTest( 11, "too few balls", 0 ) =
      7,3,4,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;		// error 7

    cUnitTest( 12, "too many pins in frame", 0 ) =
      7,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0;		// catch 2

    cUnitTest( 13, ">2 balls after final strike", 0 ) =
      X,X,X,X,X,X,X,X,X,X,X,X,X;			// catch 4

    cUnitTest( 14, ">1 ball after final spare", 0 ) =
      X,X,X,X,X,X,X,X,X,7,3,X,X;			// catch 5

    cUnitTest( 15, "too many pins for ball", 0 ) =
      11,X,X,X,X,X,X,X,X,X,X,X,X;			// catch 1

    cUnitTest( 16, "no pins and no balls", 0 );		// error 7

    cUnitTest( 17, "excess bonus score", 0 ) =
      X,X,X,X,X,X,X,X,X,X,X,11;				// catch 3 (actually 1)

    cout << string( 73, '-' ) << endl;

  }
}

int main( int argc, char **argv ) { //*****************************************

  using namespace std;

  int ret;
  stringstream ss;

  try { nTenPin::unitTests( ); }
  catch( const exception &e ) { ss << "exception: " << e.what( ) << endl; }
  catch( const    string &s ) { ss << "exception: " << s << endl; }
  catch( const    char * &s ) { ss << "exception: " << s << endl; }
  catch( ...                ) { ss << "exception: (unidentified)" << endl; }
  if( ret = ss.str( ).size( ) ) { cerr << ss.str( ); }

  return ret;
}

//*****************************************************************************
/// tenpin1.cpp <EOF>
//*****************************************************************************
