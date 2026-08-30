#include "texture_2d.h"
#include "gl_helpers.h"
#include <spdlog/spdlog.h>

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <vector>

#include <vtkAutoInit.h>
#include <vtkFloatArray.h>
#include <vtkImageActor.h>
#include <vtkImageData.h>
#include <vtkImageMapToColors.h>
#include <vtkInteractorStyleImage.h>
#include <vtkLookupTable.h>
#include <vtkPointData.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);
VTK_MODULE_INIT(vtkInteractionStyle);
VTK_MODULE_INIT(vtkRenderingFreeType);

struct PlotWindow {
    vtkSmartPointer<vtkRenderWindow> render_window;
    vtkSmartPointer<vtkRenderWindowInteractor> interactor;
};

std::vector<PlotWindow>& plot_windows() {
    static std::vector<PlotWindow> windows;
    return windows;
}

Texture2D::Texture2D(int width, int height, unsigned int internal_format, unsigned int format, unsigned int dtype)
    : m_texture_id(0), m_width(width), m_height(height), m_internal_format(internal_format), m_format(format), m_dtype(dtype) {
    GLCall(glGenTextures(1, &m_texture_id));
    this->bind();
    GLCall(glTexImage2D(GL_TEXTURE_2D, 0, internal_format, width, height, 0, format, dtype, nullptr));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
    GLCall(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
    this->unbind();
}

Texture2D::~Texture2D() {
    GLCall(glDeleteTextures(1, &m_texture_id));
}

void Texture2D::bind() const {
    SPDLOG_TRACE("Binding 2D Texture with id {}", m_texture_id);
    GLCall(glBindTexture(GL_TEXTURE_2D, m_texture_id));
}

void Texture2D::unbind() const {
    SPDLOG_TRACE("Unbinding all 2D Textures");
    GLCall(glBindTexture(GL_TEXTURE_2D, 0));
}

void Texture2D::plot_texture(std::string title) const {
    if (m_format != GL_RGBA || m_dtype != GL_FLOAT) {
        throw std::invalid_argument("plot_texture requires an RGBA floating-point texture");
    }

    GLFWwindow* previous_context = glfwGetCurrentContext();

    std::vector<float> pixels(static_cast<std::size_t>(m_width) * m_height * 4);
    this->bind();
    GLCall(glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_FLOAT, pixels.data()));
    this->unbind();

    auto render_window = vtkSmartPointer<vtkRenderWindow>::New();
    auto interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    auto image_style = vtkSmartPointer<vtkInteractorStyleImage>::New();
    interactor->SetInteractorStyle(image_style);
    interactor->SetRenderWindow(render_window);

    for (int component = 0; component < 4; ++component) {
        auto values = vtkSmartPointer<vtkFloatArray>::New();
        values->SetNumberOfComponents(1);
        values->SetNumberOfTuples(static_cast<vtkIdType>(m_width) * m_height);

        for (int y = 0; y < m_height; ++y) {
            for (int x = 0; x < m_width; ++x) {
                const std::size_t pixel = (static_cast<std::size_t>(y) * m_width + x) * 4;
                values->SetValue(static_cast<vtkIdType>(y * m_width + x), pixels[pixel + component]);
            }
        }

        auto image = vtkSmartPointer<vtkImageData>::New();
        image->SetDimensions(m_width, m_height, 1);
        image->GetPointData()->SetScalars(values);

        double range[2];
        values->GetRange(range);
        if (range[0] == range[1]) {
            range[0] -= 0.5;
            range[1] += 0.5;
        }

        auto lookup_table = vtkSmartPointer<vtkLookupTable>::New();
        lookup_table->SetNumberOfTableValues(256);
        lookup_table->SetRange(range);
        lookup_table->SetHueRange(0.667, 0.0);
        lookup_table->Build();

        auto colors = vtkSmartPointer<vtkImageMapToColors>::New();
        colors->SetInputData(image);
        colors->SetLookupTable(lookup_table);
        colors->SetOutputFormatToRGBA();
        colors->Update();

        auto actor = vtkSmartPointer<vtkImageActor>::New();
        actor->SetInputData(colors->GetOutput());

        auto renderer = vtkSmartPointer<vtkRenderer>::New();
        const int column = component % 2;
        const int row = component / 2;
        renderer->SetViewport(0.5 * column, 0.5 * row, 0.5 * (column + 1), 0.5 * (row + 1));
        renderer->AddActor(actor);
        renderer->SetBackground(0.08, 0.08, 0.1);
        renderer->ResetCamera();

        auto title = vtkSmartPointer<vtkTextActor>::New();
        title->SetInput(component == 0 ? "R" : component == 1 ? "G" : component == 2 ? "B" : "A");
        title->SetPosition(12, 12);
        title->GetTextProperty()->SetFontSize(24);
        title->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
        renderer->AddViewProp(title);
        render_window->AddRenderer(renderer);
    }

    render_window->SetSize(1200, 900);
    render_window->SetWindowName(title.c_str());
    interactor->Initialize();
    render_window->Render();

    auto& windows = plot_windows();
    windows.push_back({render_window, interactor});
    for (const PlotWindow& window : windows) {
        window.interactor->ProcessEvents();
    }
    glfwMakeContextCurrent(previous_context);
}

unsigned int Texture2D::get_texture_id() const { return m_texture_id; }
int Texture2D::get_width() const { return m_width; }
int Texture2D::get_height() const { return m_height; }
unsigned int Texture2D::get_internal_format() const { return m_internal_format; }
unsigned int Texture2D::get_format() const { return m_format; }
unsigned int Texture2D::get_dtype() const { return m_dtype; }