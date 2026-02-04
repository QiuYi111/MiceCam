from PIL import Image
import sys

def convert_to_ico(png_path, ico_path):
    try:
        img = Image.open(png_path)
        # Create sizes for the icon
        icon_sizes = [(16, 16), (32, 32), (48, 48), (64, 64), (128, 128), (256, 256)]
        img.save(ico_path, format='ICO', sizes=icon_sizes)
        print(f"Successfully converted {png_path} to {ico_path}")
    except Exception as e:
        print(f"Error converting icon: {e}")
        sys.exit(1)

if __name__ == "__main__":
    convert_to_ico("app-icon-fixed.png", "app_icon.ico")
