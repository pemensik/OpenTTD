/*
 * This file is part of OpenTTD.
 * OpenTTD is free software; you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, version 2.
 * OpenTTD is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details. You should have received a copy of the GNU General Public License along with OpenTTD. If not, see <http://www.gnu.org/licenses/>.
 */

/** @file smallmap_gui.h Smallmap GUI functions. */

#ifndef SMALLMAP_GUI_H
#define SMALLMAP_GUI_H

#include "industry_type.h"
#include "company_base.h"
#include "window_gui.h"
#include "strings_func.h"
#include "blitter/factory.hpp"
#include "linkgraph/linkgraph_gui.h"
#include "widgets/smallmap_widget.h"
#include "timer/timer.h"
#include "timer/timer_window.h"

/* set up the cargos to be displayed in the smallmap's route legend */
void BuildLinkStatsLegend();

void BuildIndustriesLegend();
void ShowSmallMap();
void BuildLandLegend();
void BuildOwnerLegend();

/** Enum for how to include the heightmap pixels/colours in small map related functions */
enum class IncludeHeightmap {
	Never,      ///< Never include the heightmap
	IfEnabled,  ///< Only include the heightmap if its enabled in the gui by the player
	Always      ///< Always include the heightmap
};

uint32_t GetSmallMapOwnerPixels(TileIndex tile, TileType t, IncludeHeightmap include_heightmap);

/** Structure for holding relevant data for legends in small map */
struct LegendAndColour {
	uint8_t colour;              ///< Colour of the item on the map.
	StringID legend;           ///< String corresponding to the coloured item.
	IndustryType type;         ///< Type of industry. Only valid for industry entries.
	uint8_t height;              ///< Height in tiles. Only valid for height legend entries.
	CompanyID company;         ///< Company to display. Only valid for company entries of the owner legend.
	bool show_on_map;          ///< For filtering industries, if \c true, industry is shown on the map in colour.
	bool end;                  ///< This is the end of the list.
	bool col_break;            ///< Perform a column break and go further at the next column.
};

/** Class managing the smallmap window. */
class SmallMapWindow : public Window {
protected:
	/** Types of legends in the #WID_SM_LEGEND widget. */
	enum SmallMapType {
		SMT_CONTOUR,
		SMT_VEHICLES,
		SMT_INDUSTRY,
		SMT_LINKSTATS,
		SMT_ROUTES,
		SMT_VEGETATION,
		SMT_OWNER,
	};

	/** Available kinds of zoomlevel changes. */
	enum ZoomLevelChange {
		ZLC_INITIALIZE, ///< Initialize zoom level.
		ZLC_ZOOM_OUT,   ///< Zoom out.
		ZLC_ZOOM_IN,    ///< Zoom in.
	};

	static SmallMapType map_type; ///< Currently displayed legends.
	static bool show_towns;       ///< Display town names in the smallmap.
	static int map_height_limit;  ///< Currently used/cached map height limit.

	static const uint INDUSTRY_MIN_NUMBER_OF_COLUMNS = 2; ///< Minimal number of columns in the #WID_SM_LEGEND widget for the #SMT_INDUSTRY legend.

	uint min_number_of_columns;    ///< Minimal number of columns in legends.
	uint min_number_of_fixed_rows; ///< Minimal number of rows in the legends for the fixed layouts only (all except #SMT_INDUSTRY).
	uint column_width;             ///< Width of a column in the #WID_SM_LEGEND widget.
	uint legend_width;             ///< Width of legend 'blob'.

	int32_t scroll_x;  ///< Horizontal world coordinate of the base tile left of the top-left corner of the smallmap display.
	int32_t scroll_y;  ///< Vertical world coordinate of the base tile left of the top-left corner of the smallmap display.
	int32_t subscroll; ///< Number of pixels (0..3) between the right end of the base tile and the pixel at the top-left corner of the smallmap display.
	int zoom;        ///< Zoom level. Bigger number means more zoom-out (further away).

	LinkGraphOverlay *overlay;

	static void BreakIndustryChainLink();
	Point SmallmapRemapCoords(int x, int y) const;

	/**
	 * Draws vertical part of map indicator
	 * @param x X coord of left/right border of main viewport
	 * @param y Y coord of top border of main viewport
	 * @param y2 Y coord of bottom border of main viewport
	 */
	static inline void DrawVertMapIndicator(int x, int y, int y2)
	{
		GfxFillRect(x, y,      x, y + 3, PC_VERY_LIGHT_YELLOW);
		GfxFillRect(x, y2 - 3, x, y2,    PC_VERY_LIGHT_YELLOW);
	}

	/**
	 * Draws horizontal part of map indicator
	 * @param x X coord of left border of main viewport
	 * @param x2 X coord of right border of main viewport
	 * @param y Y coord of top/bottom border of main viewport
	 */
	static inline void DrawHorizMapIndicator(int x, int x2, int y)
	{
		GfxFillRect(x,      y, x + 3, y, PC_VERY_LIGHT_YELLOW);
		GfxFillRect(x2 - 3, y, x2,    y, PC_VERY_LIGHT_YELLOW);
	}

	/**
	 * Compute minimal required width of the legends.
	 * @return Minimally needed width for displaying the smallmap legends in pixels.
	 */
	inline uint GetMinLegendWidth() const
	{
		return WidgetDimensions::scaled.framerect.left + this->min_number_of_columns * this->column_width;
	}

	/**
	 * Return number of columns that can be displayed in \a width pixels.
	 * @return Number of columns to display.
	 */
	inline uint GetNumberColumnsLegend(uint width) const
	{
		return width / this->column_width;
	}

	/**
	 * Compute height given a number of columns.
	 * @param num_columns Number of columns.
	 * @return Needed height for displaying the smallmap legends in pixels.
	 */
	inline uint GetLegendHeight(uint num_columns) const
	{
		return WidgetDimensions::scaled.framerect.Vertical() +
				this->GetNumberRowsLegend(num_columns) * FONT_HEIGHT_SMALL;
	}

	/**
	 * Get a bitmask for company links to be displayed. Usually this will be
	 * the _local_company. Spectators get to see all companies' links.
	 * @return Company mask.
	 */
	inline CompanyMask GetOverlayCompanyMask() const
	{
		return Company::IsValidID(_local_company) ? 1U << _local_company : MAX_UVALUE(CompanyMask);
	}

	/** Blink the industries (if selected) on a regular interval. */
	IntervalTimer<TimerWindow> blink_interval = {std::chrono::milliseconds(450), [this](auto) {
		Blink();
	}};

	/** Update the whole map on a regular interval. */
	IntervalTimer<TimerWindow> refresh_interval = {std::chrono::milliseconds(930), [this](auto) {
		ForceRefresh();
	}};

	void RebuildColourIndexIfNecessary();
	uint GetNumberRowsLegend(uint columns) const;
	void SelectLegendItem(int click_pos, LegendAndColour *legend, int end_legend_item, int begin_legend_item = 0);
	void SwitchMapType(SmallMapType map_type);
	void SetNewScroll(int sx, int sy, int sub);

	void DrawMapIndicators() const;
	void DrawSmallMapColumn(void *dst, uint xc, uint yc, int pitch, int reps, int start_pos, int end_pos, Blitter *blitter) const;
	void DrawVehicles(const DrawPixelInfo *dpi, Blitter *blitter) const;
	void DrawTowns(const DrawPixelInfo *dpi) const;
	void DrawSmallMap(DrawPixelInfo *dpi) const;

	Point RemapTile(int tile_x, int tile_y) const;
	Point PixelToTile(int px, int py, int *sub, bool add_sub = true) const;
	Point ComputeScroll(int tx, int ty, int x, int y, int *sub);
	void SetZoomLevel(ZoomLevelChange change, const Point *zoom_pt);
	void SetOverlayCargoMask();
	void SetupWidgetData();
	uint32_t GetTileColours(const TileArea &ta) const;

	int GetPositionOnLegend(Point pt);

	void UpdateLinks();
	void Blink();
	void ForceRefresh();

public:
	friend class NWidgetSmallmapDisplay;

	SmallMapWindow(WindowDesc *desc, int window_number);
	virtual ~SmallMapWindow();

	void SmallMapCenterOnCurrentPos();
	Point GetStationMiddle(const Station *st) const;

	void Close([[maybe_unused]] int data = 0) override;
	void SetStringParameters(int widget) const override;
	void OnInit() override;
	void OnPaint() override;
	void DrawWidget(const Rect &r, int widget) const override;
	void OnClick([[maybe_unused]] Point pt, int widget, [[maybe_unused]] int click_count) override;
	void OnInvalidateData(int data = 0, bool gui_scope = true) override;
	bool OnRightClick(Point pt, int widget) override;
	void OnMouseWheel(int wheel) override;
	void OnScroll(Point delta) override;
	void OnMouseOver([[maybe_unused]] Point pt, int widget) override;
};

#endif /* SMALLMAP_GUI_H */
