/* -*-mode:c++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */

#ifndef VP8_RASTER_H
#define VP8_RASTER_H

#include <iostream>

#include "config.h"
#include "raster.hh"

#ifdef HAVE_SSE2
#include "predictor_sse.hh"
#endif

class MotionVector;

template <class integer>
static inline uint8_t clamp255( const integer value )
{
  if ( value < 0 ) return 0;
  if ( value > 255 ) return 255;
  return value;
}

class VP8Raster : public BaseRaster
{
public:
  template <unsigned int size>
  class Block
  {
  public:
    typedef TwoDSubRange<uint8_t, size, 1> Row;
    typedef TwoDSubRange<uint8_t, 1, size> Column;

  private:
    TwoDSubRange<uint8_t, size, size> contents_;
    unsigned int column_, row_;

    void dc_predict() { dc_predict( this->contents_ ); }
    void dc_predict_simple() { dc_predict_simple( this->contents_ ); }
    void vertical_predict() { vertical_predict( this->contents_ ); }
    void horizontal_predict() { horizontal_predict( this->contents_ ); }
    void true_motion_predict() { true_motion_predict( this->contents_ ); }
    void vertical_smoothed_predict() { vertical_smoothed_predict( this->contents_ ); }
    void horizontal_smoothed_predict() { horizontal_smoothed_predict( this->contents_ ); }
    void left_down_predict() { left_down_predict( this->contents_ ); }
    void right_down_predict() { right_down_predict( this->contents_ ); }
    void vertical_right_predict() { vertical_right_predict( this->contents_ ); }
    void vertical_left_predict() { vertical_left_predict( this->contents_ ); }
    void horizontal_down_predict() { horizontal_down_predict( this->contents_ ); }
    void horizontal_up_predict() { horizontal_up_predict( this->contents_ ); }

    void dc_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void dc_predict_simple( TwoDSubRange<uint8_t, size, size> & output ) const;
    void vertical_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void horizontal_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void true_motion_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void vertical_smoothed_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void horizontal_smoothed_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void left_down_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void right_down_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void vertical_right_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void vertical_left_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void horizontal_down_predict( TwoDSubRange<uint8_t, size, size> & output ) const;
    void horizontal_up_predict( TwoDSubRange<uint8_t, size, size> & output ) const;

    struct Predictors {
      static const Row & row127( void );
      static const Column & col129( void );

      const Row above_row;
      const Column left_column;
      const uint8_t & above_left;

      /* non-const so macroblock can adjust the rightmost subblocks */
      struct AboveRightBottomRowPredictor {
        Row above_right_bottom_row;
        const uint8_t * above_bottom_right_pixel;
        bool use_row;

        uint8_t above_right( const unsigned int column ) const;
      } above_right_bottom_row_predictor;

      uint8_t above( const int8_t column ) const;
      uint8_t left( const int8_t row ) const;
      uint8_t east( const int8_t num ) const;

      Predictors( TwoD<uint8_t> & component,
                  const unsigned int column,
                  const unsigned int row );
    } predictors_;

  public:
    uint8_t above( const int column ) const { return predictors_.above( column ); }
    uint8_t left( const int column ) const { return predictors_.left( column ); }
    uint8_t east( const int column ) const { return predictors_.east( column ); }

    unsigned int column() const { return column_; }
    unsigned int row() const { return row_; }

    Block( const unsigned int column, const unsigned int row, TwoD<uint8_t> & raster_component );

    /* XXX will go away */
    Block( const typename TwoD<Block>::Context & c,
           const unsigned int column_offset, const unsigned int row_offset,
           TwoD<uint8_t> & raster_component );

    uint8_t & at( const unsigned int column, const unsigned int row )
    { return contents_.at( column, row ); }

    const uint8_t & at( const unsigned int column, const unsigned int row ) const
    { return contents_.at( column, row ); }

    unsigned int stride( void ) const { return contents_.stride(); }

    const decltype(contents_) & contents( void ) const { return contents_; }
    decltype(contents_) & mutable_contents( void ) { return contents_; }
    const Predictors & predictors( void ) const { return predictors_; }

    template <class PredictionMode>
    void intra_predict( const PredictionMode mb_mode ) { intra_predict( mb_mode, this->contents_ ); }

    template <class PredictionMode>
    void intra_predict( const PredictionMode mb_mode,
                        TwoDSubRange<uint8_t, size, size> & output ) const;

    /* for encoder use */
    template <class PredictionMode>
    void intra_predict( const PredictionMode mb_mode, TwoD<uint8_t> & output ) const;

    void inter_predict( const MotionVector & mv,
                        const TwoD<uint8_t> & reference ) { inter_predict( mv, reference, this->contents_ ); }

    void inter_predict( const MotionVector & mv,
                        const TwoD<uint8_t> & reference,
                        TwoDSubRange<uint8_t, size, size> & output ) const;

    /* for encoder use */
    void inter_predict( const MotionVector & mv,
                        const TwoD<uint8_t> & reference,
                        TwoD<uint8_t> & output ) const;

    void analyze_inter_predict( const MotionVector & mv, const TwoD<uint8_t> & reference );

    template <class ReferenceType>
    void safe_inter_predict( const MotionVector & mv,
                             const ReferenceType & reference,
                             const int source_column, const int source_row,
                             TwoDSubRange<uint8_t, size, size> & output ) const;

    void unsafe_inter_predict( const MotionVector & mv,
                               const TwoD<uint8_t> & reference,
                               const int source_column, const int source_row,
                               TwoDSubRange<uint8_t, size, size> & output ) const;

#ifdef HAVE_SSE2
    static void sse_horiz_inter_predict( const uint8_t * src, const unsigned int pixels_per_line,
                                         const uint8_t * dst, const unsigned int dst_pitch,
                                         const unsigned int dst_height, const unsigned int filter_idx );

    static void sse_vert_inter_predict( const uint8_t * src, const unsigned int pixels_per_line,
                                        const uint8_t * dst, const unsigned int dst_pitch,
                                        const unsigned int dst_height, const unsigned int filter_idx );
#endif

    void set_above_right_bottom_row_predictor( const typename Predictors::AboveRightBottomRowPredictor & replacement );

    static constexpr unsigned int dimension { size };

    SafeArray<SafeArray<int16_t, size>, size> operator-( const Block & other ) const;

    friend std::ostream &operator<<( std::ostream & output, const Block<size> & block )
    {
      for ( size_t i = 0; i < size; i++ ) {
        for ( size_t j = 0; j < size; j++ ) {
          std::cout << static_cast<int>( block.contents().at( i, j ) );
          if ( j != size - 1 or i != size - 1 ) std::cout << ", ";
        }
      }

      return output;
    }
  };

  using Block4  = Block<4>;
  using Block8  = Block<8>;
  using Block16 = Block<16>;

  struct Macroblock
  {
    Block16 & Y;
    Block8 & U;
    Block8 & V;
    TwoDSubRange<Block4, 4, 4> Y_sub;
    TwoDSubRange<Block4, 2, 2> U_sub, V_sub;

    Macroblock( const TwoD<Macroblock>::Context & c, VP8Raster & raster );

    bool operator==( const Macroblock & other ) const
    {
      return Y.contents() == other.Y.contents()
         and U.contents() == other.U.contents()
         and V.contents() == other.V.contents();
    }

    bool operator!=( const Macroblock & other ) const
    {
      return not operator==( other );
    }

    Macroblock & operator=( const Macroblock & other ) = delete;
  };

private:
  TwoD<Block4> Y_subblocks_ { width_ / 4, height_ / 4, 0, 0, Y_ },
    U_subblocks_ { width_ / 8, height_ / 8, 0, 0, U_ },
    V_subblocks_ { width_ / 8, height_ / 8, 0, 0, V_ };

  TwoD<Block16> Y_bigblocks_ { width_ / 16, height_ / 16, 0, 0, Y_ };
  TwoD<Block8>  U_bigblocks_ { width_ / 16, height_ / 16, 0, 0, U_ };
  TwoD<Block8>  V_bigblocks_ { width_ / 16, height_ / 16, 0, 0, V_ };

  TwoD<Macroblock> macroblocks_ { width_ / 16, height_ / 16, *this };

public:
  VP8Raster( const unsigned int display_width, const unsigned int display_height );

  Macroblock & macroblock( const unsigned int column, const unsigned int row )
  {
    return macroblocks_.at( column, row );
  }

  const Macroblock & macroblock( const unsigned int column, const unsigned int row ) const
  {
    return macroblocks_.at( column, row );
  }

  static unsigned int macroblock_dimension( const unsigned int num ) { return ( num + 15 ) / 16; }

  template <class lambda>
  void macroblocks_forall_ij( const lambda & f ) const
  {
    for ( unsigned int row = 0; row < macroblocks_.height(); row++ ) {
      for ( unsigned int column = 0; column < macroblocks_.width(); column++ ) {
        f( macroblock( column, row ), column, row );
      }
    }
  }
};

class EdgeExtendedRaster
{
private:
  const TwoD<uint8_t> & master_;

public:
  EdgeExtendedRaster( const TwoD<uint8_t> & master )
    : master_( master ) {}

  uint8_t at( const int column, const int row ) const
  {
    int bounded_column = column;
    if ( bounded_column < 0 ) bounded_column = 0;
    if ( bounded_column > int(master_.width() - 1) ) bounded_column = master_.width() - 1;

    int bounded_row = row;
    if ( bounded_row < 0 ) bounded_row = 0;
    if ( bounded_row > int(master_.height() - 1) ) bounded_row = master_.height() - 1;

    return master_.at( bounded_column, bounded_row );
  }
};

#endif //
