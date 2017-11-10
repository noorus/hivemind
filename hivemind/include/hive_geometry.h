#pragma once

namespace hivemind {

  namespace bgi = boost::geometry::index;

  using BoostPoint = boost::geometry::model::d2::point_xy<double>;
  using BoostPolygon = boost::geometry::model::polygon<BoostPoint>;

  using BoostSegment = boost::geometry::model::segment<BoostPoint>;
  using BoostSegmentI = std::pair < BoostSegment, std::size_t >;

  typedef boost::polygon::voronoi_diagram<double> BoostVoronoi;
  typedef boost::polygon::point_data<double> VoronoiPoint;
  typedef boost::polygon::segment_data<double> VoronoiSegment;

  typedef boost::geometry::model::linestring<BoostPoint> BoostLine;
  typedef boost::geometry::model::multi_linestring<BoostLine> BoostMultiLine;
  typedef boost::geometry::model::multi_polygon<BoostPolygon> BoostMultiPoly;

}